#pragma once
#include "../config.h"
#include "../storage/record.hpp"
#include "../core/database.hpp"
#include "../core/cache.hpp"

#include <mutex>

//first 4KB block is the header, which contains metadata about the WAL
struct WALHeaderBlock
{
    off_t current_LSN; //the offset in the WAL file where the next log entry will be written
    off_t last_checkpoint_LSN; //the offset in the WAL file where the last checkpoint was made
};

struct MapBlock
{
    off_t page_location[WAL_ENTRIES_PER_MAP_PAGE]; //the offset in the database file where the page is located
};




class WAL
{
private:
    int wal_file_fd = -1;

public:
    size_t current_entry_index = 0;
    WAL() = default;

    void log_insert(Page& page, off_t page_location);

    void recover(Database& database);

private:
    void build_wal();

};
