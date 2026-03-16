#pragma once
#include "../config.h"
#include "../query/plan/planner.hpp"
#include "../core/cache.hpp"
#include <unordered_set>
#include "manager.hpp"

class WAL;

class Transaction {
private:
    std::unordered_map<off_t, Page> pages;
    Cache& cache;
    LockManager &lock_manager;


public:
    size_t txn_id;
    WAL &wal;



    std::unordered_set<off_t> locks_held; // List of page locations for which this transaction holds locks

    std::unordered_set<off_t> shared_locks_held;

    std::unordered_set<off_t> temp_locks;

    //@On commit, these blocks are allocated and used by the transaction
    //@On roll back, these blocks are freed and used in the linked list of free blocks.
    std::vector<off_t> allocated_blocks;

    //@On commit, these blocks are freed
    //@On roll back, nothing happnes to them.
    std::vector<off_t> freed_blocks;

    Transaction(int txn_id, Cache& cache, LockManager &lock_manager, WAL &wal) : txn_id(txn_id), cache(cache), lock_manager(lock_manager), wal(wal) {};

    void begin();

    int commit();

    void rollback();


    //@ must be called on any mutable pages
    int copy_page(off_t page_location);

    //@ check is exlusive locked
    bool is_page_owned(off_t page_location);

    int copy_page_no_lock(off_t page_location);

    bool acquire_ownership_and_copy_if_needed(off_t page_location);

    void try_acquire_shared(off_t page_location);

    void try_release_shared(off_t page_location);

    //@ mutable
    Page* private_cache_read(off_t page_location);

    //@Returns a pointer to the page in LRU cache
    Page* read_page(off_t page_location);

    //private cache only write
    void write_to_page(Page* page, size_t offset, const void* src, size_t len, off_t block_offset);

    void acquire_shared(off_t page_location);

    void release_shared(off_t page_location);


    //btree stuff
    void try_temp_lock(off_t page_location);

    void try_release_temp_lock(off_t page_location);

    void promote_temp_locks_to_permanent();

    void release_temp_locks();

};
