#pragma once

#include "../transactions/transaction.hpp"
#include "file.hpp"
class PostingList
{
private:
    File *file;
public:

    std::string key;
    off_t root_location;
    Transaction& txn;

    off_t seeker_location;
    Posting_Block *seeker_block;

    PostingList(std::string key, off_t root_location, Transaction& txn, File *file)
    : file(file),
    key(key), root_location(root_location),
    txn(txn),
    seeker_location(root_location),
    seeker_block(nullptr) {};


    void lock_and_insert(off_t location);
};
