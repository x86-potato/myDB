#include "wal.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string.h>
constexpr int    WAL_PAGE_LIMIT          = 65536;
constexpr int    WAL_FILE_SIZE           = 4096 + WAL_PAGE_LIMIT * 4096; // in bytes
constexpr auto   WAL_FLUSH_INTERVAL      = std::chrono::milliseconds(50);
constexpr double WAL_CHECKPOINT_THRESHOLD = 0.75; // checkpoint when ring is this full
WAL::WAL(File *file) : file(file)
{
    int fd = open(WAL_LOG_FILE_NAME.c_str(), O_RDWR | O_CREAT, 0644);

    if (fd == -1) {
        perror("open");
        throw std::runtime_error("Failed to open WAL log file");
    }

    wal_file_fd = fd;

    // If file size is 0, initialize the file to full capacity
    if (lseek(wal_file_fd, 0, SEEK_END) == 0) {
        build_wal();
    }

    // Always read the header into memory on boot to initialize our pointers
    if (pread(wal_file_fd, &header, sizeof(WALHeaderBlock), 0) == -1) {
        throw std::runtime_error("Failed to read WAL header on initialization");
    }
    
    // Resume absolute index from wherever the last checkpoint left off
    current_entry_index = header.checkpoint_lsn;

    // Start background flusher (WAL group commit)
    flusher_thread = std::thread(&WAL::flusher_loop, this);

    // Start background writer (drains dirty cache pages to DB file)
    bgwriter_thread = std::thread(&WAL::bgwriter_loop, this);
}

WAL::~WAL()
{
    {
        std::lock_guard<std::mutex> lock(flush_cv_mutex);
        stop_flusher = true;
    }
    flush_cv.notify_one();
    bgwriter_cv.notify_one();
    if (flusher_thread.joinable()) {
        flusher_thread.join();
    }
    if (bgwriter_thread.joinable()) {
        bgwriter_thread.join();
    }
    if (wal_file_fd != -1) {
        close(wal_file_fd);
        wal_file_fd = -1;
    }
}

void WAL::bgwriter_loop()
{
    constexpr auto BGWRITER_INTERVAL = std::chrono::milliseconds(200);

    while (true) {
        {
            std::unique_lock<std::mutex> lock(flush_cv_mutex);
            bgwriter_cv.wait_for(lock, BGWRITER_INTERVAL);
            if (stop_flusher) break;
        }

        // Write dirty pages to DB file (pwrite only, no fdatasync).
        // Checkpoint's fdatasync will harden these writes when it runs.
        // flush_cache() swaps the dirty list atomically so concurrent
        // checkpoint calls get disjoint page sets — no double-writes.
        file->cache.flush_cache();
    }
}

void WAL::flusher_loop()
{
    while (true) {
        {
            std::unique_lock<std::mutex> cv_lock(flush_cv_mutex);
            flush_cv.wait_for(cv_lock, WAL_FLUSH_INTERVAL);
            if (stop_flusher) break;
        }

        // Light: sync WAL ring buffer to disk — drains OS buffer for all pending
        // log_insert calls without holding wal_mutex (group commit).
        fdatasync(wal_file_fd);

        // Heavy: full checkpoint only when WAL ring is getting full.
        uint64_t active;
        {
            std::lock_guard<std::mutex> lock(wal_mutex);
            active = current_entry_index - header.checkpoint_lsn;
        }
        if (active > static_cast<uint64_t>(WAL_PAGE_LIMIT * WAL_CHECKPOINT_THRESHOLD)) {
            checkpoint();
        }
    }
}

void WAL::log_insert(std::unordered_map<off_t, Page> &pages)
{
    std::unique_lock<std::mutex> lock(wal_mutex);

    if (wal_file_fd == -1) {
        throw std::runtime_error("WAL log file is not open");
    }

    // 1. CAPACITY CHECK: only count pages that need a NEW slot (not already tracked).
    auto count_new = [&]() -> uint64_t {
        uint64_t n = 0;
        for (const auto &[pid, _] : pages) {
            auto it = page_lsn_map.find(pid);
            if (it == page_lsn_map.end() || it->second < header.checkpoint_lsn) n++;
        }
        return n;
    };

    uint64_t active = current_entry_index - header.checkpoint_lsn;
    if (active + count_new() + 1 >= WAL_PAGE_LIMIT) {
        flush_cv.notify_one();
        checkpoint_done_cv.wait(lock, [&] {
            return (current_entry_index - header.checkpoint_lsn + count_new() + 1)
                   < static_cast<uint64_t>(WAL_PAGE_LIMIT);
        });
    }

    // 2. WRITE PAGES with deduplication.
    // If the page already has a slot in the active WAL region, overwrite it in-place
    // (same physical slot, current_entry_index does NOT advance).
    // Only new pages consume a fresh ring slot.
    for (const auto &[page_id, page] : pages) {
        off_t physical_slot;
        auto it = page_lsn_map.find(page_id);
        if (it != page_lsn_map.end() && it->second >= header.checkpoint_lsn) {
            // Known page: overwrite its existing slot.
            physical_slot = static_cast<off_t>(it->second % WAL_PAGE_LIMIT);
        } else {
            // New page: allocate the next ring slot.
            physical_slot = static_cast<off_t>(current_entry_index % WAL_PAGE_LIMIT);
            page_lsn_map[page_id] = current_entry_index;
            current_entry_index++;
        }

        off_t write_offset = physical_slot * PAGE_SIZE + 4096;
        if (pwrite(wal_file_fd, page.buffer, PAGE_SIZE, write_offset) == -1) {
            perror("pwrite");
            throw std::runtime_error("Failed to write page to WAL log file");
        }
    }

    // 3. ADVANCE TERMINATOR to the slot immediately after the last written entry.
    static const char terminator_block[PAGE_SIZE] = {0};
    off_t term_offset = (current_entry_index % WAL_PAGE_LIMIT) * PAGE_SIZE + 4096;
    pwrite(wal_file_fd, terminator_block, PAGE_SIZE, term_offset);

    // NOTE: fdatasync intentionally omitted. The background flusher calls
    // fdatasync(wal_file_fd) every WAL_FLUSH_INTERVAL (group commit).
    if (sync_every_commit) {
        fdatasync(wal_file_fd);
    }
}

void WAL::checkpoint()
{
    // === PHASE 1: snapshot (under wal_mutex, no I/O) ===
    uint64_t lsn_snapshot;
    {
        std::lock_guard<std::mutex> lock(wal_mutex);
        if (current_entry_index <= header.checkpoint_lsn) return; // nothing to do
        if (checkpoint_in_progress) return;                        // already running
        checkpoint_in_progress = true;
        lsn_snapshot = current_entry_index;
    }

    std::cout << "[WAL] Initiating Circular Checkpoint...\n";

    // === PHASE 2: flush dirty pages to DB file (NO wal_mutex held) ===
    try {
        // flush_cache() internally swaps the dirty list (fast, under its own lock)
        // then does the pwrite calls (slow) — all without wal_mutex.
        file->cache.flush_cache();

        if (fdatasync(file->file_fd) == -1) {
            throw std::runtime_error("Failed to fsync main database file during checkpoint");
        }
    } catch (...) {
        {
            std::lock_guard<std::mutex> lock(wal_mutex);
            checkpoint_in_progress = false;
        }
        checkpoint_done_cv.notify_all();
        throw;
    }

    // === PHASE 3a: advance checkpoint_lsn IN MEMORY, wake blocked writers ===
    // DB pages are durable. Writers can immediately reuse freed ring slots.
    // The on-disk WAL header is updated below in Phase 3b.
    {
        std::lock_guard<std::mutex> lock(wal_mutex);
        header.checkpoint_lsn = lsn_snapshot;
    }
    checkpoint_done_cv.notify_all();

    // === PHASE 3b: persist checkpoint_lsn to WAL header on disk ===
    // This is fast (4096 bytes) and runs while writers are already unblocked.
    {
        std::lock_guard<std::mutex> lock(wal_mutex);
        if (pwrite(wal_file_fd, &header, sizeof(WALHeaderBlock), 0) == -1) {
            checkpoint_in_progress = false;
            perror("pwrite");
            throw std::runtime_error("Failed to update WAL header block");
        }

        if (fdatasync(wal_file_fd) == -1) {
            checkpoint_in_progress = false;
            throw std::runtime_error("Failed to fsync WAL header");
        }

        checkpoint_in_progress = false;
    }

    std::cout << "[WAL] Circular Checkpoint complete.\n";
}

void WAL::build_wal()
{
    // Set file size to WAL_FILE_SIZE (allocate all blocks up front)
    if (ftruncate(wal_file_fd, WAL_FILE_SIZE) == -1) {
        perror("ftruncate");
        throw std::runtime_error("Failed to allocate WAL log file size");
    }
    
    off_t file_size = lseek(wal_file_fd, 0, SEEK_END);
    if (file_size != WAL_FILE_SIZE) {
        throw std::runtime_error("Failed to set WAL log file size");
    }

    // Initialize header
    WALHeaderBlock new_header;
    new_header.checkpoint_lsn = 0;

    if (pwrite(wal_file_fd, &new_header, sizeof(WALHeaderBlock), 0) == -1) {
        perror("pwrite");
        throw std::runtime_error("Failed to write WAL header block");
    }
}

void WAL::recover(Database& database)
{
    // Re-read header just to be absolutely sure we are in sync
    if (pread(wal_file_fd, &header, sizeof(WALHeaderBlock), 0) == -1) {
        perror("pread");
        throw std::runtime_error("Failed to read WAL header block for recovery");
    }

    std::cout << "Starting recovery from absolute LSN: " << header.checkpoint_lsn << "\n";

    // Start tracking from the tail
    current_entry_index = header.checkpoint_lsn;

    void* buffer;
    if (posix_memalign(&buffer, 512, PAGE_SIZE) != 0) {
        throw std::runtime_error("Failed to allocate aligned memory for buffer");
    }

    // We can recover at most WAL_PAGE_LIMIT pages before hitting our own tail
    for(size_t i = 0; i < WAL_PAGE_LIMIT; i++) {
        
        off_t physical_index = current_entry_index % WAL_PAGE_LIMIT;
        off_t entry_location = physical_index * PAGE_SIZE + 4096;

        if (pread(wal_file_fd, buffer, PAGE_SIZE, entry_location) == -1) {
            perror("pread");
            free(buffer);
            throw std::runtime_error("Failed to read page entry from WAL log file");
        }

        // Read first 8 bytes as page id
        off_t page_id = *reinterpret_cast<off_t*>(buffer);

        // Read second 8 bytes as additional validation field
        uint64_t second_qword = *(reinterpret_cast<uint64_t*>(
            static_cast<char*>(buffer) + sizeof(uint64_t)
        ));

        // THE TERMINATOR BLOCK: Entry is considered empty ONLY if both are zero
        if (page_id == 0 && second_qword == 0) {
            std::cout << "Hit terminator block at absolute LSN: " << current_entry_index << "\n";
            break; // Recovery finished!
        }
        
        // Write recovered page directly to the main database file
        file->write_page_to_file(buffer, page_id);
        std::cout << "Recovered page with id: " << page_id << " from WAL\n"; 

        current_entry_index++;
    }

    free(buffer);
    
    // Clear out any half-baked cache state
    file->cache.clear();

    // Force a checkpoint! This secures all the recovered pages to the main DB file
    // and officially updates the WAL header to the end of the recovered data.
    checkpoint(); 
}

void WAL::debug(off_t to_find)
{
    void* buffer;
    if (posix_memalign(&buffer, 512, PAGE_SIZE) != 0) {
        throw std::runtime_error("Failed to allocate aligned memory for buffer");
    }

    // Note: Debug searches the entire physical file space
    for(int i = 0; i < WAL_PAGE_LIMIT; i++) {
        off_t entry_location = i * PAGE_SIZE + 4096;
        if (pread(wal_file_fd, buffer, PAGE_SIZE, entry_location) == -1) {
            continue; // Keep searching even on error
        }

        off_t page_id = *reinterpret_cast<off_t*>(buffer);
        if(page_id == to_find) {
            std::cout << "Found page with id: " << page_id << " at physical index " << i << "\n";
        }
    }
    free(buffer);
}