#include "manager.hpp"


void LockManager::acquire_shared(int txn_id, off_t page_location)
{
    cache.cache_lock.lock(); // Lock the cache to ensure thread safety while accessing the page
    NodeLRU* node = cache.lru.page_to_node[page_location];
    cache.cache_lock.unlock(); // Unlock the cache after accessing the page

    node->rw_latch.lock_shared();
}

void LockManager::acquire_exclusive(int txn_id, off_t page_location)
{

    cache.cache_lock.lock(); // Lock the cache to ensure thread safety while accessing the page
    NodeLRU* node = cache.lru.page_to_node[page_location];
    cache.cache_lock.unlock(); // Unlock the cache after accessing the page


    node->rw_latch.lock();
      
}


void LockManager::release_shared(int txn_id, off_t page_location)
{
    cache.cache_lock.lock(); // Lock the cache to ensure thread safety while accessing the page
    NodeLRU* node = cache.lru.page_to_node[page_location];
    cache.cache_lock.unlock(); // Unlock the cache after accessing the page

    node->rw_latch.unlock_shared();
}

void LockManager::release_exclusive(int txn_id, off_t page_location)
{
    cache.cache_lock.lock(); 
    NodeLRU* node = cache.lru.page_to_node[page_location];
    cache.cache_lock.unlock(); 
    node->rw_latch.unlock();
}