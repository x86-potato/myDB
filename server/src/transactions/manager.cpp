#include "manager.hpp"


void LockManager::acquire_shared(int txn_id, off_t page_location)
{
    (void)txn_id;
    cache.read_block(page_location);

    NodeLRU* node;
    {
        std::lock_guard<std::mutex> lock(cache.cache_lock);
        node = cache.lru.page_to_node[page_location];
    }

    node->rw_latch.lock_shared();
}

void LockManager::acquire_ownership(int txn_id, off_t page_location)
{
    (void)txn_id;
    NodeLRU* node;
    {
        std::lock_guard<std::mutex> lock(cache.cache_lock);
        node = cache.lru.page_to_node[page_location];
    }

    node->owner_mutex.lock();
}

void LockManager::acquire_exclusive(int txn_id, off_t page_location)
{
    (void)txn_id;
    NodeLRU* node;
    {
        std::lock_guard<std::mutex> lock(cache.cache_lock);
        node = cache.lru.page_to_node[page_location];
    }

    node->rw_latch.lock();
}

void LockManager::release_exclusive(int txn_id, off_t page_location)
{
    (void)txn_id;
    NodeLRU* node;
    {
        std::lock_guard<std::mutex> lock(cache.cache_lock);
        node = cache.lru.page_to_node[page_location];
    }

    node->rw_latch.unlock();
}

void LockManager::release_shared(int txn_id, off_t page_location)
{
    (void)txn_id;
    NodeLRU* node;
    {
        std::lock_guard<std::mutex> lock(cache.cache_lock);
        node = cache.lru.page_to_node[page_location];
    }

    node->rw_latch.unlock_shared();
}

void LockManager::release_ownership(int txn_id, off_t page_location)
{
    (void)txn_id;
    NodeLRU* node;
    {
        std::lock_guard<std::mutex> lock(cache.cache_lock);
        node = cache.lru.page_to_node[page_location];
    }

    node->owner_mutex.unlock();
}
