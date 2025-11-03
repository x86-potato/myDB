#pragma once

#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>


#include "config.h"
#include "cache.hpp"
#include "table.hpp"
#include "record.hpp"





#define HEADER_TABLE_LOCATION 0
#define HEADER_ROOT_NODE_LOCATION 8
#define HEADER_ROOT_DATA_LOCATION 16

// Forward declarations to avoid circular dependencies


enum status {
    EMPTY,
    READY
};

struct Data_Node {
    off_t fill_ptr = 0;
    off_t next_free_block = 0;
    off_t overflow = false;
    
    std::byte padding[4072] = {std::byte(0)};
};

class File {
public:
    std::string file_name = "database.db"; 
    int file_fd;
    status file_status = status::EMPTY; 

    Cache cache;

    off_t header_pointer;
    off_t table_block_pointer;
    off_t root_node_pointer;
    off_t free_data_pointer;

    int index_block_count = 0;
    int data_blocks_count = 0;

    Table primary_table;

    File();

    template<typename MyBtree, typename NodeT, typename InternalNodeT, typename LeafNodeT>    
    void insert_data(std::string key, Record &record, MyBtree index_tree);

    template<typename MyBtree, typename NodeT, typename InternalNodeT, typename LeafNodeT>    
    Record find(std::string key, MyBtree index_tree);

    void update_root_pointer();

    off_t alloc_block();

    template<typename NodeT>
    void update_node(NodeT *node, off_t node_location);

    template<typename LeafNodeT>
    void update_leafnode(LeafNodeT *node, off_t node_location);

    template<typename NodeT, typename InternalNodeT, typename LeafNodeT>
    NodeT* load_node(off_t disk_offset);

    template<typename BtreeT>
    void print_leaves(off_t disk_node_offset);

    off_t insert_table(Table *table);
    Table load_table();

private:
    void header_block_creation();
    void init_table_block();

    template<typename NodeT>
    void init_root_node();

    void init_data_node(off_t location);

    off_t get_table_block();
    off_t get_root_node();
    off_t get_data();

    int open_file();

    Record get_record(off_t record_location);

    off_t write_record(Record &record);
    Data_Node *load_data_node(off_t location);
};

