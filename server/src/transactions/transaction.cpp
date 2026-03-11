#include "transaction.hpp"

void Transaction::begin() {
    //std::cout << "Transaction " << txn_id << " started.\n";
}

//@ must be called on any mutable pages
int Transaction::copy_page(off_t page_location) {
    //this must be removed later.
    if(pages.find(page_location) != pages.end()) {
        std::cout << "Page " << page_location << " is already in the transaction's local buffer.\n";
        return 0; // Page is already in the transaction's local buffer
    }
    Page* page = cache.read_block(page_location);  



    this->pages[page_location] = *page; // Copy the page into the transaction's local buffer



    return 0; // Return 0 on success, or an error code if needed
}


//@ check is exlusive locked
bool Transaction::is_page_locked_exclusive(off_t page_location) {

    return locks_held.find(page_location) != locks_held.end();
}


//@ copy page without acquiring lock, used for read only pages
bool Transaction::acquire_exclusive_and_copy_if_needed(off_t page_location) {
    if(!is_page_locked_exclusive(page_location)) {
        if(copy_page(page_location) == 0) {
            lock_manager.acquire_exclusive(txn_id, page_location);  
            locks_held.insert(page_location);
            return true; // Lock was successfully acquired
        }
        return false; // Failed to acquire lock
    }
    return true; // Lock is already held by this transaction
}

int Transaction::copy_page_no_lock(off_t page_location) {
    Page* page = cache.read_block(page_location);  

    cache.cache_lock.lock(); // Lock the cache to ensure thread safety while accessing the page
    NodeLRU* node = cache.lru.page_to_node[page_location];
    cache.cache_lock.unlock(); // Unlock the cache after accessing the page

    //node->rw_latch.lock(); // Lock the page for exclusive access

    this->pages[page_location] = *page; // Copy the page into the transaction's local buffer

    this->locks_held.insert(page_location); // Record that this transaction holds a lock on the page


    return 0; // Return 0 on success, or an error code if needed
}

//@ mutable
Page* Transaction::private_cache_read(off_t page_location) {
    auto it = pages.find(page_location);
    if (it != pages.end()) {
        return &it->second;
    }
    return nullptr; // Return nullptr if the page is not found
}

const Page* Transaction::read_page(off_t page_location) {
    auto it = pages.find(page_location);
    if (it != pages.end()) {
        return &it->second; // Return pointer to the page in the transaction's local buffer
    }
    return nullptr; // Return nullptr if the page is not in the transaction's local buffer
}

void Transaction::write_to_page(Page* page, size_t offset, const void* src, size_t len, off_t block_offset) {
    assert(offset + len <= BLOCK_SIZE);
    
    assert(pages.find(block_offset) != pages.end()); // Ensure the page is in the transaction's local buffer

    assert(locks_held.find(block_offset) != locks_held.end()); // Ensure the transaction holds a lock on the page

   memcpy(page->buffer + offset, src, len); // Write to the page in the transaction's local buffer

}

void Transaction::acquire_shared(off_t page_location)
{
    lock_manager.acquire_shared(txn_id, page_location);
    // No need to track shared locks in the transaction's local state for this implementation
}
void Transaction::release_shared(off_t page_location)
{
    lock_manager.release_shared(txn_id, page_location);
    
}


int Transaction::commit() {
    for (off_t block_offset : locks_held) {
        auto it = pages.find(block_offset);
        if (it != pages.end()) {
            Page* page = cache.read_block(block_offset); // Read the current page from the cache
            std::cout << "Committing page at offset " << block_offset << " for transaction " << txn_id << ".\n";
            cache.write_to_page(page, 0, &it->second, BLOCK_SIZE, block_offset); // Write the modified page back to the cache
        }
        else
        {
            std::cout << "Error: Transaction " << txn_id << " does not have page at offset " << block_offset << " in its local buffer.\n";
        }
    }
   for (off_t block_offset : locks_held) {
        cache.cache_lock.lock(); // Lock the cache to ensure thread safety while accessing the page
        NodeLRU* node = cache.lru.page_to_node[block_offset];
        cache.cache_lock.unlock(); // Unlock the cache after accessing the page

        node->rw_latch.unlock(); // Unlock the page after writing
    }

    std::cout << "Transaction " << txn_id << " committed.\n";


    return 0; // Return 0 on success, or an error code if needed
}


void Transaction::try_temp_lock(off_t page_location)
{
    if (locks_held.count(page_location) || temp_locks.count(page_location)) {
        return;
    }

    lock_manager.acquire_exclusive(txn_id, page_location);
    temp_locks.insert(page_location);
}
void Transaction::try_release_temp_lock(off_t page_location)
{
    if (temp_locks.count(page_location)) {
        lock_manager.release_exclusive(txn_id, page_location);
        temp_locks.erase(page_location);
    }
}
void Transaction::promote_temp_locks_to_permanent()
{
    for (off_t loc : temp_locks) {
        locks_held.insert(loc);
        pages[loc] = *cache.read_block(loc); // Copy the page into the transaction's local buffer
    }
    temp_locks.clear();
}

void Transaction::release_temp_locks()
{
    temp_locks.clear();
}