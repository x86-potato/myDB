#include "wal.hpp"
#include <mutex>
#include <unordered_map>


WAL::WAL(File *file) : file(file)
{
    int fd = open(WAL_LOG_FILE_NAME.c_str(), O_RDWR | O_CREAT, 0644);

    if (fd == -1) {
        perror("open");
        throw std::runtime_error("Failed to open WAL log file");
    }

    wal_file_fd = fd;

    //if file size is 0, call to create structure
    if (lseek(wal_file_fd, 0, SEEK_END) == 0)
        build_wal();

    //allocate file size based on config



}

void WAL::log_insert(std::unordered_map<off_t, Page> &pages)
{
    std::lock_guard<std::mutex> lock(wal_mutex);

    if (wal_file_fd == -1) {
        throw std::runtime_error("WAL log file is not open");
    }

    if(current_entry_index >= WAL_PAGE_LIMIT) {
        throw std::runtime_error("WAL log is full");
    }

    for (const auto &page : pages) {
        //write page data to wal file at the correct offset
        off_t entry_location = current_entry_index * PAGE_SIZE + 4096;
        if (pwrite(wal_file_fd, page.second.buffer, PAGE_SIZE, entry_location) == -1) {
            perror("pwrite");
            throw std::runtime_error("Failed to write page to WAL log file");
        }
        current_entry_index++;
    }


}

void WAL::debug(off_t to_find)
{
    void* buffer;
    if (posix_memalign(&buffer, 512, PAGE_SIZE) != 0) {
        throw std::runtime_error("Failed to allocate aligned memory for buffer");
    }

    for(int i = 0; i < WAL_PAGE_LIMIT; i++) {
        if (pread(wal_file_fd, buffer, PAGE_SIZE, to_find) == -1) {
            perror("pread");
            free(buffer);
            throw std::runtime_error("Failed to read from WAL log file");
        }

        //read first 8 bytes as page id
        off_t page_id = *reinterpret_cast<off_t*>(buffer);
        if(page_id == to_find) {
            std::cout << "Found page with id: " << page_id << std::endl;
        }
    }
    free(buffer);
}


void WAL::build_wal()
{
    //set file size to WAL_FILE_SIZE
    if (ftruncate(wal_file_fd, WAL_FILE_SIZE) == -1) {
        perror("ftruncate");
        throw std::runtime_error("Failed to allocate WAL log file size");
    }
    //make sure file size is correct
    off_t file_size = lseek(wal_file_fd, 0, SEEK_END);
    if (file_size != WAL_FILE_SIZE) {
        throw std::runtime_error("Failed to set WAL log file size");
    }
    //wal is structred in 2 sections

    //1. header
    WALHeaderBlock header;
    if (pwrite(wal_file_fd, &header, sizeof(WALHeaderBlock), 0) == -1) {
        perror("pwrite");
        throw std::runtime_error("Failed to write WAL header block");
    }
}

void WAL::recover(Database& database)
{
    //read header block to find last checkpoint LSN
    WALHeaderBlock header;
    if (pread(wal_file_fd, &header, sizeof(WALHeaderBlock), 0) == -1) {
        perror("pread");
        throw std::runtime_error("Failed to read WAL header block");
    }

    if(header.last_checkpoint_LSN == -1) {
        std::cout << "No checkpoint found, starting recovery from the beginning of the WAL" << std::endl;
        header.last_checkpoint_LSN = 4096; //start of first page entry
    } else {
        std::cout << "Found checkpoint at LSN: " << header.last_checkpoint_LSN << ", starting recovery from there" << std::endl;
    }

    //read page entries from WAL starting from last checkpoint LSN
    off_t start_LSN = header.last_checkpoint_LSN;
    size_t start_index = (start_LSN - 4096) / PAGE_SIZE; //calculate starting page index based on LSN

    void* buffer;
    if (posix_memalign(&buffer, 512, PAGE_SIZE) != 0) {
        throw std::runtime_error("Failed to allocate aligned memory for buffer");
    }

    for(size_t i = start_index; i < WAL_PAGE_LIMIT; i++) {
        off_t entry_location = i * PAGE_SIZE + 4096;
        if (pread(wal_file_fd, buffer, PAGE_SIZE, entry_location) == -1) {
            perror("pread");
            free(buffer);
            throw std::runtime_error("Failed to read page entry from WAL log file");
        }

        //read first 8 bytes as page id
        off_t page_id = *reinterpret_cast<off_t*>(buffer);
        if(page_id == 0) {
            //no more valid entries in the WAL
            break;
        }

        //create page object and add to database cache
        
        file->write_page_to_file(buffer, page_id); //write page data back to file at the correct offset
        std::cout << "Recovered page with id: " << page_id << " from WAL" << std::endl; 

        file->cache.clear();
        header.last_checkpoint_LSN = entry_location; //update checkpoint LSN to the last successfully recovered page
        current_entry_index = i + 1; //update current entry index to continue writing after recovered entries

    }
    if(pwrite(wal_file_fd, &header, sizeof(WALHeaderBlock), 0) == -1) {
        perror("pwrite");
        free(buffer);
        throw std::runtime_error("Failed to update WAL header block after recovery");
    }


}