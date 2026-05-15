#pragma once
#include "../core/cache.hpp"
#include "../storage/file.hpp"
#include <atomic>
#include <condition_variable>
#include <fcntl.h>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <unordered_map>

class Database;
class File;

// First 4KB block is the header, which contains metadata about the WAL
struct WALHeaderBlock
{
    // Now represents the ABSOLUTE page index, not a physical byte offset
    uint64_t checkpoint_lsn = 0; 
};

class WAL
{
private:
    int wal_file_fd = -1;
    std::mutex wal_mutex; // Prevent multiple threads from writing to the WAL at the same time
    File *file;
    
    // Cache the header in memory so we don't have to read it during inserts
    WALHeaderBlock header;

    // Background flusher thread (group commit — fdatasync WAL every 50ms)
    std::thread flusher_thread;
    std::atomic<bool> stop_flusher{false};
    std::condition_variable flush_cv;
    std::mutex flush_cv_mutex;

    // Background writer thread (drains dirty cache pages to DB file continuously)
    std::thread bgwriter_thread;
    std::condition_variable bgwriter_cv;

    // Two-phase checkpoint coordination
    bool checkpoint_in_progress = false;
    std::condition_variable checkpoint_done_cv; // associated with wal_mutex

    // Deduplication: page_id -> absolute LSN of its current WAL slot.
    // If the LSN is < checkpoint_lsn the entry is stale and a new slot is allocated.
    std::unordered_map<off_t, uint64_t> page_lsn_map;

public:
    size_t current_entry_index = 0; // Absolute counter for the ring buffer

    // When true, every log_insert calls fdatasync before returning.
    // Guarantees per-transaction durability at the cost of throughput.
    bool sync_every_commit = false;

    WAL(File *file);
    ~WAL();

    void log_insert(std::unordered_map<off_t, Page> &pages);
    void debug(off_t to_find);

    void recover(Database& database);

private:
    void build_wal();
    void checkpoint();
    void flusher_loop();
    void bgwriter_loop();
};