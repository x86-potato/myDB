#pragma once
#include "../config.h"
#include "../storage/record.hpp"
#include "../core/database.hpp"
#include "../core/cache.hpp"

#include <mutex>


struct MapBlock
{
    struct WalEntry
    {
        off_t page_location;
        off_t log_offset;
    };

    WalEntry entries[WAL_ENTRIES_PER_MAP_PAGE];      
};



class WAL
{
private:
    int wal_file_fd = -1;
    
public:
    WAL() = default;

    void log_insert(Page& page, off_t page_location);

    void recover(Database& database);

private:
    void build_wal();

};