//responsible for mamaging locks done via transactions
#pragma once
#include "../config.h"
#include "../core/cache.hpp"
#include <mutex>
#include <unordered_map>

class LockManager {
public:
    LockManager(Cache &cache) : cache(cache) {}

    void acquire_shared(int txn_id, off_t page_location);
    void acquire_exclusive(int txn_id, off_t page_location);
    void release_shared(int txn_id, off_t page_location);
    void release_exclusive(int txn_id, off_t page_location);

private:
    std::mutex lock_mutex; // Mutex for synchronizing access to the lock manager's internal state

    Cache &cache;
    //@maps page location to the txn id
    std::unordered_map<off_t, int> txn_locks; // Map of page locations to their corresponding locks
};