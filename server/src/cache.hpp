#pragma once

#include "config.h"
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <cassert>
#include <cstring>


// Structure representing a memory page
struct Page
{
    std::byte buffer[BLOCK_SIZE] = {std::byte(0)};
};

// Node used for LRU tracking
struct NodeLRU
{
    NodeLRU* next = nullptr;
    NodeLRU* prev = nullptr;

    Page* page_ptr = nullptr;
    off_t block_ptr = 0;
    bool dirty = false;
};

class Cache
{
public:
    int filefd;
    int cache_miss_counter = 0;
    int cache_hit_counter = 0;

    Cache();
    ~Cache();

    Page* read_block(off_t block_off);
    int write_block(off_t block_off);
    Page* insert(off_t block_offset);

    void write_to_page(Page* page, size_t offset, const void* src, size_t len, off_t block_offset);
    void flush_cache();

private:
    std::byte* cache_start;
    bool cache_index_in_use[CACHE_PAGE_LIMIT] = {false};
    std::unordered_map<off_t, Page*> page_table;

    class LRU
    {
    public:
        NodeLRU nodes[CACHE_PAGE_LIMIT];
        std::unordered_map<off_t, NodeLRU*> page_to_node;
        int allocated_count = 0;
        NodeLRU *head = nullptr;
        NodeLRU *tail = nullptr;
        Cache *cache;

        LRU();

        Page* insert(Page* page, off_t block_offset, std::unordered_map<off_t, Page*>& page_table);
        void move_to_head(NodeLRU* node);
        void flush_all();
        void print_LRU_list();
    };

    LRU lru;

    std::byte* pre_allocate(size_t bytes);
};
