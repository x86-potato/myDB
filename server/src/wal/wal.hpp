#pragma once
#include "../core/cache.hpp"
#include "../storage/file.hpp"
#include <fcntl.h>
#include <unistd.h>

class Database;
class File;

#include <mutex>

//first 4KB block is the header, which contains metadata about the WAL
struct WALHeaderBlock
{
    off_t last_checkpoint_LSN = -1; //the offset in the WAL file where the last checkpoint was made
};





class WAL
{
private:
    int wal_file_fd = -1;
    std::mutex wal_mutex; //prevent multiple threads from writing to the WAL at the same time
    File *file;

public:
    size_t current_entry_index = 0;
    WAL(File *file);

    void log_insert(std::unordered_map<off_t, Page> &pages);
    void debug(off_t to_find);

    void recover(Database& database);

private:
    void build_wal();

};
