#ifndef BTREE_HPP
#define BTREE_HPP

#include <iostream>
#include <cassert>
#include <cstring>
#include <string>

#define PAGE_SIZE 256 //256 bytes
#define MAX_KEYS 100

// Forward declaration - breaks circular dependency
class File;

struct Node {
    off_t disk_location = 0;
    char keys[MAX_KEYS][32] = { 0 };
    uint16_t current_key_count = 0;
    bool is_leaf = true;
    off_t parent = 0;
};

struct internal_node : Node {
    off_t children[MAX_KEYS + 1];
}; 

struct leaf_node : Node { 
    off_t values[MAX_KEYS] = {0}; 
    off_t next_leaf = 0; 
};

struct insert_up_data {
    char key[32];
    off_t left_child = 0;
    off_t right_child = 0;
};

// Utility function declaration
bool is_less_than(const char left[32], const char right[32]);

class BtreePlus { 
public: 
    BtreePlus(File &file);
    
    void insert(std::string insert_string, off_t value);
    leaf_node* find_leftmost_leaf(Node* root);
    
    Node* root_node;
    File &file;
    int degree = 3;

private:
    off_t get_next_node_pointer(char* to_insert, internal_node *node);
    int find_left_node_child_index(Node *node);
    void insert_up_into(insert_up_data data, off_t node_location);
    void split_leaf(Node* node);
    void split_internal(Node* node);
    void insert_key_into_node(insert_up_data data, Node* node);
};

int func(BtreePlus &myTree, File &file);

#endif // BTREE_HPP