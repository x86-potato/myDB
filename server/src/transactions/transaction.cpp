#include "transaction.hpp"
#include "../wal/wal.hpp"

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
bool Transaction::is_page_owned(off_t page_location) {

    return locks_held.count(page_location) ||
           temp_locks.count(page_location);

}


//@ copy page without acquiring lock, used for read only pages
bool Transaction::acquire_ownership_and_copy_if_needed(off_t page_location) {
    if(!is_page_owned(page_location)) {
        // 1. Lock it FIRST so no one else can modify the global cache!
        lock_manager.acquire_ownership(txn_id, page_location);
        
        // 2. NOW copy it safely to your private transaction cache
        if(copy_page(page_location) == 0) {
            locks_held.insert(page_location);
            return true;
        }
        // If copy fails, release the lock
        lock_manager.release_ownership(txn_id, page_location);
        return false;
    }
    return true; 
}

int Transaction::copy_page_no_lock(off_t page_location) {
    Page* page = cache.read_block(page_location);

    cache.cache_lock.lock(); // Lock the cache to ensure thread safety while accessing the page
    NodeLRU* node = cache.lru.page_to_node[page_location];
    (void)node;
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


Page* Transaction::read_page(off_t page_location) {
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

void Transaction::try_release_ownership(off_t page_location)
{
    if (locks_held.count(page_location))
    {
        lock_manager.release_ownership(txn_id, page_location);
        locks_held.erase(page_location);
    }
}

void Transaction::try_acquire_shared(off_t page_location)
{
    if (shared_locks_held.count(page_location)) {
        return; // Already holds a lock on this page
    }

    lock_manager.acquire_shared(txn_id, page_location);
    shared_locks_held.insert(page_location);
}

void Transaction::try_release_shared(off_t page_location)
{
    if (!shared_locks_held.count(page_location))
    {
        return;
    }

    lock_manager.release_shared(txn_id, page_location);
    shared_locks_held.erase(page_location);

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

void Transaction::print_txn_stat()
{
    std::cout << "Transaction " << txn_id << " holds locks on pages: ";
    for (off_t block_offset : locks_held) {
        std::cout << block_offset << " ";
    }
    std::cout << "\n";
}

int Transaction::commit() {

    // 1. WRITE-AHEAD LOG: Flush all changes to disk FIRST.
    // If the server crashes during this call, the transaction simply rolls back.
    if (!pages.empty()) {
        wal.log_insert(pages);
    }

    // 2. APPLY TO CACHE: The data is safe on disk. We can now update RAM.
    for (off_t block_offset : locks_held) {
        auto it = pages.find(block_offset);
        if (it != pages.end()) {
            
            // Acquire exclusive to prevent readers from seeing half-written bytes
            lock_manager.acquire_exclusive(txn_id, block_offset);
            
            Page* page = cache.read_block(block_offset); 
            //std::cout << "Committing page at offset " << block_offset << " for transaction " << txn_id << ".\n";
            cache.write_to_page(page, 0, &it->second, BLOCK_SIZE, block_offset); 
            
            lock_manager.release_exclusive(txn_id, block_offset); 
        }
        else {
            std::cout << "Error: TXN " << txn_id << " missing page " << block_offset << " in local buffer.\n";
        }
    }

    // 3. VISIBILITY: Release ownership locks so other threads can see the durable data.
    for (off_t block_offset : locks_held) {
        lock_manager.release_ownership(txn_id, block_offset);
    }
    locks_held.clear();

    //std::cout << "Transaction " << txn_id << " committed safely.\n";
    return 0; 
}


void Transaction::rollback()
{
    // Release all exclusive ownership locks (write locks from B+tree ops)
    for (off_t page : locks_held) {
        lock_manager.release_ownership(txn_id, page);
    }
    locks_held.clear();

    // Release any temporary ownership locks (B+tree structural locks)
    for (off_t page : temp_locks) {
        lock_manager.release_ownership(txn_id, page);
    }
    temp_locks.clear();

    // Release shared (read) locks
    for (off_t page : shared_locks_held) {
        lock_manager.release_shared(txn_id, page);
    }
    shared_locks_held.clear();

    // Discard private page buffer — changes are abandoned, not written to cache or WAL
    pages.clear();
}

void Transaction::try_temp_lock(off_t page_location)
{
    if (locks_held.count(page_location) || temp_locks.count(page_location)) {
        return;
    }

    lock_manager.acquire_ownership(txn_id, page_location);
    temp_locks.insert(page_location);
}
void Transaction::try_release_temp_lock(off_t page_location)
{
    if (temp_locks.count(page_location)) {
        lock_manager.release_ownership(txn_id, page_location);
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
    for (off_t loc : temp_locks) {
    lock_manager.release_ownership(txn_id, loc);
    }
    temp_locks.clear();
}
