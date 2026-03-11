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
}


void WAL::build_wal()
{

}
