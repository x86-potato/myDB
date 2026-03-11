#pragma once
#include "../config.h"
#include "../storage/record.hpp"
#include "../core/database.hpp"
#include "../core/cache.hpp"

#include <mutex>


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