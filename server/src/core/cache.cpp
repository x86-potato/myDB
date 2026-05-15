#include "../config.h"
#include "cache.hpp"


Cache::Cache(){
    cache_start = pre_allocate(CACHE_PAGE_LIMIT * BLOCK_SIZE);
    //cache_start = static_cast<std::byte*>(malloc (CACHE_PAGE_LIMIT * BLOCK_SIZE));
    lru.cache = this;
    page_table.reserve(CACHE_PAGE_LIMIT);
    lru.page_to_node.reserve(CACHE_PAGE_LIMIT);
    std::cout << "\nprealloc done\n:";
}

void Cache::clear()
{
    for (int i = 0; i < CACHE_PAGE_LIMIT; i++)
    {
        cache_index_in_use[i] = false;
    }
    page_table.clear();
    lru.page_to_node.clear();
    lru.allocated_count = 0;
    lru.head = nullptr;
    lru.tail = nullptr;

    std::lock_guard<std::mutex> lg(dirty_list_mutex);
    dirty_page_list.clear();
}

int Cache::write_block(off_t block_off)
{
    auto find = page_table.find(block_off);
    if(find == page_table.end())
    {
        perror("block not in memory"); 
    }
    else
    {
        assert(find->second != nullptr);
        ssize_t written = pwrite(filefd, find->second, BLOCK_SIZE, block_off);
        if (written == -1) {
            perror("pwrite failed");
        } else if (written != BLOCK_SIZE) {
            fprintf(stderr, "pwrite: partial write (%zd/%d bytes)\n", written, BLOCK_SIZE);
        }
    }
    return 0;
}


Page* Cache::insert(off_t block_offset)
{
    Page *p = nullptr;
    for (int i = 0; i < CACHE_PAGE_LIMIT; i++)
    {
        if(!cache_index_in_use[i])
        {
            p = new (cache_start + (BLOCK_SIZE * i)) Page();   
            cache_index_in_use[i] = true;
            lru.insert(p, block_offset, page_table);
            cache_miss_counter--;
            break;
        }
    }

    if (!p) // cache full
    {
        p = lru.insert(p, block_offset, page_table);
    }
    //std::cout << "fetch ";

    if (pread(filefd, p, BLOCK_SIZE, block_offset) != BLOCK_SIZE) {
        //std::cout << "disk fetch ";
        perror("pread");
    }

    page_table[block_offset] = p;
    return p;
}

Page* Cache::read_block(off_t block_off)
{
    std::lock_guard<std::mutex> lock(cache_lock);

    auto find = page_table.find(block_off);
    if(find == page_table.end())
    {
        cache_miss_counter++;
        Page* output = insert(block_off);
        return output;      
    }
    else
    {
        NodeLRU* node = lru.page_to_node[block_off];
        lru.move_to_head(node);
        cache_hit_counter++;
        return find->second;
    }
}

void Cache::pin_page(off_t block_off)
{
    auto find = page_table.find(block_off);
    if(find != page_table.end())
    {
        NodeLRU* node = lru.page_to_node[block_off];
        node->pin_count++;
    }
}

void Cache::unpin_page(off_t block_off)
{
    auto find = page_table.find(block_off);
    if(find != page_table.end())
    {
        NodeLRU* node = lru.page_to_node[block_off];
        if (node->pin_count > 0) {
            node->pin_count--;
        }
    }
}

void Cache::write_to_page(Page* page, size_t offset, const void* src, size_t len, off_t block_offset) {
    assert(offset + len <= BLOCK_SIZE);
    memcpy(page->buffer + offset, src, len);

    NodeLRU* node = lru.page_to_node[block_offset];
    assert(node);
    if (node) node->dirty = true;

    {
        std::lock_guard<std::mutex> lg(dirty_list_mutex);
        dirty_page_list.insert(block_offset);
    }
}

void Cache::WAL()
{

}

void Cache::flush_cache()
{
    //std::cout << "\nhits:" << cache_hit_counter << " misses: " << cache_miss_counter;

    std::unordered_set<off_t> to_flush;
    {
        std::lock_guard<std::mutex> lg(dirty_list_mutex);
        to_flush.swap(dirty_page_list);
    }

    for (off_t offset : to_flush) {
        write_block(offset);
        NodeLRU* node = lru.page_to_node[offset];
        if (node) node->dirty = false;
    }
}

std::byte* Cache::pre_allocate(size_t bytes)
{
    std::byte* ptr = (std::byte*)(mmap(nullptr, bytes,
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    -1, 0));
    if (ptr == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
    return ptr;
}


LRU::LRU() : allocated_count(0), head(nullptr), tail(nullptr), cache(nullptr) {}

Page* LRU::insert(Page* page, off_t block_offset, std::unordered_map<off_t, Page*>& page_table)
{
    if (head == nullptr) 
    {
        //nodes[0] = NodeLRU{nullptr, nullptr, page, block_offset, false, 0, std::shared_mutex()};
        nodes[0].page_ptr = page;
        nodes[0].block_ptr = block_offset;
        
        head = &nodes[0];
        tail = &nodes[0];
        allocated_count = 1;
    } 
    else if (allocated_count < CACHE_PAGE_LIMIT) 
    {
        //nodes[allocated_count] = NodeLRU{head, nullptr, page, block_offset, false, 0, std::shared_mutex()};
        nodes[allocated_count].page_ptr = page;
        nodes[allocated_count].block_ptr = block_offset;
        nodes[allocated_count].next = head;
        nodes[allocated_count].prev = nullptr;

        if (head) head->prev = &nodes[allocated_count];
        head = &nodes[allocated_count];
        allocated_count++;
    }
    else 
    {
        NodeLRU* to_evict = tail;
        if (to_evict->dirty) cache->write_block(to_evict->block_ptr);

        tail = to_evict->prev;
        if (tail) tail->next = nullptr;

        page_table.erase(to_evict->block_ptr);

        page = to_evict->page_ptr;
        to_evict->block_ptr = block_offset;
        to_evict->dirty = false;

        to_evict->prev = nullptr;
        to_evict->next = head;
        if (head) head->prev = to_evict;
        head = to_evict;
        if (!tail) tail = head;
    }

    page_to_node[block_offset] = head;
    return head->page_ptr;
}

void LRU::move_to_head(NodeLRU* node) {
    if (node == head) return;  
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
    if (node == tail) tail = node->prev;

    node->prev = nullptr;
    node->next = head;
    if (head) head->prev = node;
    head = node;
}

void LRU::flush_all() {
    for (auto& node : nodes) {
        if (node.page_ptr && node.dirty) {
            cache->write_block(node.block_ptr);
            node.dirty = false;
        }
    }
}

void LRU::print_LRU_list() 
{
    NodeLRU* curr = head;
    std::cout << "LRU List (head -> tail):\n";
    while (curr) {
        std::cout << curr->page_ptr << " -> ";
        curr = curr->next;
    }
    std::cout << "NULL\n";
}
