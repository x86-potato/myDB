#include "../storage/file.hpp"
#include "transaction.hpp"
#include <cstddef>


struct SharedPageGuard {
    //RAII guard for a page read from cache.
    //
    SharedPageGuard(off_t page_location, Transaction& txn) : page_location(page_location), txn(&txn) {
        txn.acquire_shared(page_location);
    }

    //delete
    ~SharedPageGuard() {
        txn.release_shared(page_location);
    }

    //delete move constructor
    SharedPageGuard(SharedPageGuard&& other) = delete;

    //implement move assignment operator
    SharedPageGuard& operator=(SharedPageGuard&& other)
    {
        if (this == &other)
            return *this;

        //release current page if any
        txn->release_shared(page_location);

        //move data from other
        txn = other.txn;
        page_location = other.page_location;

        //invalidate other
        other.txn = nullptr;
        other.page_location = -1;

        return *this;
    }

    //delete copy constructor
    SharedPageGuard(const SharedPageGuard& other) = delete;

    //init copy assignment operator
    SharedPageGuard& operator=(const SharedPageGuard& other) = delete;


    Transaction* txn = nullptr;
    off_t page_location = -1;
    Page* page = nullptr;
};
