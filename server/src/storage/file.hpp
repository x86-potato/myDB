#pragma once

#include <cstdint>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>


#include "../config.h"
#include "../core/cache.hpp"
#include "../core/table.hpp"
#include "record.hpp"
#include "../core/btree.hpp"
#include "../transactions/transaction.hpp"
#include "../transactions/manager.hpp"



#define HEADER_TABLE_LOCATION 0
#define HEADER_ROOT_DATA_LOCATION 8
#define DATA_NODE_METADATA_SIZE 32

// Forward declarations to avoid circular dependencies


class Database;

enum class BlockType : uint8_t {
    INTERNAL_NODE,
    LEAF_NODE,
    DATA_NODE
};

enum status {
    EMPTY,
    READY
};

struct Header_Block
{
    off_t location;
    off_t table_linked_list_head;
};

struct Data_Node {
    off_t location = 0; // used for wal
    off_t fill_ptr = 0;
    off_t next_free_block = 0;
    off_t overflow = false;

    std::byte padding[4064] = {std::byte(0)};
};

struct Table_Block
{
    off_t location = 0; // used for wal
    off_t next_table_block = -1;
};

struct Posting_Block
{
    off_t location = 0; // used for wal
    off_t next = 0;
    off_t prev = 0;
    off_t entries[508] = {0}; // 508 entries of 8 bytes each = 4064 bytes
    uint32_t size = 0;
    int32_t free_index = -1;
};


struct Freed_Block
{
    off_t location = 0; // used for wal
    off_t next_free_block;
    BlockType original_type;
};



class File {
public:
    std::string file_name = "database.db";
    int file_fd;
    status file_status = status::EMPTY;

    Cache cache;

    off_t header_pointer;
    off_t free_linked_list_head = 0;
    off_t table_linked_list_head = -1;

    int index_block_count = 0;
    int data_blocks_count = 0;



    Database *database;
    LockManager *lock_manager;

    std::mutex allocation_mutex; // Mutex for synchronizing block allocation


    File();
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    File(File&&) = delete;
    File& operator=(File&&) = delete;


    void write_page_to_file(const void* data, off_t offset);

    template <typename MyBtree>
    off_t insert_primary_index(
        std::string key,Record &record, MyBtree &tree, Table &table, int txn_id);

    template <typename MyBtree>
    void insert_secondary_index(std::string key, Table &table, MyBtree &tree, off_t record_location, int index, int txn_id);

    template <typename PrimaryTree>
    void parse_primary_tree(PrimaryTree &tree);

    template <typename PrimaryBtree, typename MyBtree32, typename MyBtree16, typename MyBtree8, typename MyBtree4>
    void build_secondary_index(Table& table, int columnIndex,
                                    PrimaryBtree* primaryTree,
                                    MyBtree32* index32,
                                    MyBtree16* index16,
                                    MyBtree8*  index8,
                                    MyBtree4*  index4,
                                    Transaction& txn);

    template<typename MyBtree32, typename MyBtree16, typename MyBtree8, typename MyBtree4>
    void generate_index(int columnIndex, Table& table,
                          MyBtree32* index32,
                          MyBtree16* index16,
                          MyBtree8*  index8,
                          MyBtree4*  index4,
                          Transaction& txn);

    template<typename MyBtree, typename NodeT, typename InternalNodeT, typename LeafNodeT>
    std::vector<Record> find(std::string key, MyBtree &index_tree, off_t root_location, Table &table);


    off_t alloc_block();

    template<typename NodeT>
    void update_node(NodeT *node, off_t node_location, size_t size, Transaction &txn);

    template<typename LeafNodeT>
    void update_leafnode(LeafNodeT *node, off_t node_location);

    void update_root_pointer(Table* table, off_t old_location, off_t new_location);

    void update_root_pointer(off_t old_location, off_t new_location);

    template<typename NodeT>
    NodeT* load_node(off_t disk_offset, Transaction& txn);


    template<typename BtreeT>
    void print_leaves(off_t disk_node_offset);

    off_t update_table_index_location(Table &table, int column_index, off_t new_index_value);

    template <typename Node32,typename Node16, typename Node8, typename Node4>
    off_t insert_table(Table &table, int txn_id);
    std::vector<Table> load_tables();

    int update_record(Record &original_record,off_t location, int column_index, std::string &value, Table* table, int txn_id);
    off_t write_record(Record &record, int txn_id, Table& table);
    Record get_record(off_t record_location, const Table& table, Transaction* txn = nullptr);
    int delete_record(const Record &record, off_t location, const Table& table, int txn_id);

    Posting_Block *load_posting_block(off_t location);

private:
    void header_block_creation();
    void init_table_block(off_t location);

    template<typename NodeT>
    void init_node(off_t location, Transaction& txn);

    void init_data_node(off_t location, int txn_id);

    void init_posting_block(off_t location);
    void update_posting_block(off_t location, Posting_Block *block);
    void delete_from_posting_list(off_t posting_block_location, off_t record_location);

    off_t get_table_block();
    off_t get_root_node();
    off_t get_data();

    int open_file();


    Data_Node *load_data_node(off_t location);
};
