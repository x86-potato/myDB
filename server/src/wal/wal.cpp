#include "wal.hpp"


WAL::WAL()
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

void WAL::log_insert(Page& page, off_t page_location)
{

}


void WAL::build_wal()
{
    //set file size to WAL_FILE_SIZE
    if (ftruncate(wal_file_fd, WAL_FILE_SIZE) == -1) {
        perror("ftruncate");
        throw std::runtime_error("Failed to allocate WAL log file size");
    }
    //wal is structred in 3 sections

    //1. header
    //2. map pages
    //3. page data
}
