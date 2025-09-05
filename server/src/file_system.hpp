#ifndef FILE_HPP
#define FILE_HPP

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>

#define BLOCK_SIZE 4096

enum status {
    EMPTY,
    READY
};

// Forward declarations - breaks circular dependency
class BtreePlus;
struct Node;

class File {
public:
    File();
    
    int add_data_to(char* data, int length, int block_id, off_t block_offset);
    off_t alloc_block();
    void insert_data(BtreePlus &tree, std::string key, int value);
    void read_node_block(off_t block_pointer);
    void header_block_creation(FILE* file);
    void update_leaf_node(Node *node, off_t node_location);
    void update_node(Node *node, off_t node_location);
    Node *load_node(off_t disk_offset);

    std::string file_name = "database.db"; 
    int index_block_count;
    int data_blocks_count;
    status file_status = status::EMPTY; 
    off_t header_pointer;
    off_t root_node_pointer;
    void print_leaves(off_t disk_node_offset);
    Node cast_node(off_t root_node);

private:
    void init_root_node(FILE* file);
    off_t get_root_node(FILE* file);
    int open_file(FILE* file);
};

#endif // FILE_HPP