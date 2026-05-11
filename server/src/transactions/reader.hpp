#include "../storage/file.hpp"
#include "transaction.hpp"
#include <cstddef>

class SharedPageGuard {
public:
    SharedPageGuard() : txn(nullptr), page_location(-1), page(nullptr) {}

    // 2. Main Constructor
    SharedPageGuard(off_t page_location, Transaction& txn) 
        : page_location(page_location), txn(&txn) 
    {
        this->txn->acquire_shared(page_location);

        page = this->txn->read_page(page_location);
        if (page == nullptr) {
            // FIXED: Actually assign the result of the cache read to the page
            page = this->txn->cache.read_block(page_location); 
        }
    }

    // 3. Destructor
    ~SharedPageGuard() {
        if (txn != nullptr && page_location != -1) {
            txn->release_shared(page_location);
        }
    }

    // 4. Move Constructor (Required to return this guard from functions)
    SharedPageGuard(SharedPageGuard&& other) noexcept 
        : txn(other.txn), page_location(other.page_location), page(other.page) 
    {
        // Invalidate the other guard so its destructor does nothing
        other.txn = nullptr;
        other.page_location = -1;
        other.page = nullptr;
    }

    // 5. Move Assignment Operator
    SharedPageGuard& operator=(SharedPageGuard&& other) noexcept
    {
        if (this == &other)
            return *this;

        // Release current page if we already own one
        if (txn != nullptr && page_location != -1) {
            txn->release_shared(page_location);
        }

        // Move data from other
        txn = other.txn;
        page_location = other.page_location;
        page = other.page;

        // Invalidate other
        other.txn = nullptr;
        other.page_location = -1;
        other.page = nullptr;

        return *this;
    }

    // 6. Delete Copy semantics (Locks cannot be copied!)
    SharedPageGuard(const SharedPageGuard& other) = delete;
    SharedPageGuard& operator=(const SharedPageGuard& other) = delete;

    // 7. Smart Pointer Accessors
    Page* operator->() const { return page; }
    Page* get() const { return page; }
    off_t location() const { return page_location; }

private:
    Transaction* txn = nullptr;
    off_t page_location = -1;
    Page* page = nullptr;
};