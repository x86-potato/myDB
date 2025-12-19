#pragma once
#include <iostream>
#include <cassert>
#include <cstring>
#include <string>


#include "../config.h"
#include "../storage/file.hpp"


template<size_t KeySize, size_t MaxKeys>
struct Node {
    off_t disk_location = 0;
    char keys[MaxKeys][KeySize] = { 0 };
    uint16_t current_key_count = 0;
    bool is_leaf = true;
    off_t parent = 0;
};
template<size_t KeySize, size_t MaxKeys>
struct InternalNode : Node<KeySize, MaxKeys> {
    off_t children[MaxKeys + 1] = { 0 };
    //char padding[3904] = { 0 };
};
template<size_t KeySize, size_t MaxKeys>
struct LeafNode : Node<KeySize, MaxKeys> {
    off_t values[MaxKeys] = { 0 };
    off_t next_leaf = 0;
    //char padding[3904] = { 0 };
};
using Node32 = Node<32,MaxKeys_32>;
using Node16 = Node<16, MaxKeys_16>;
using Node8 =  Node<8, MaxKeys_8>;
using Node4 =  Node<4, MaxKeys_4>;

using InternalNode32 = InternalNode<32,MaxKeys_32>;
using InternalNode16 = InternalNode<16,MaxKeys_16>;
using InternalNode8 = InternalNode<8,MaxKeys_8>;
using InternalNode4 = InternalNode<4, MaxKeys_4>;

using LeafNode32 = LeafNode<32,MaxKeys_32>;
using LeafNode16 = LeafNode<16,MaxKeys_16>;
using LeafNode8 = LeafNode<8,MaxKeys_8>;
using LeafNode4 = LeafNode<4, MaxKeys_4>;


template<typename LeafNodeT>
struct LocationData
{
    LeafNodeT leaf;
    int key_index;
};


class File;
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
class BtreePlus {
public:
    using NodeType = NodeT;
    using LeafNodeType = LeafNodeT;
    using InternalNodeType = InternalNodeT;
    NodeT* root_node = nullptr;
    File *file = nullptr;

    off_t tree_root = 0;
    static constexpr int MaxKeys= []() {
        if constexpr (std::is_same_v<NodeT, Node8>) return MaxKeys_8;
        else if constexpr (std::is_same_v<NodeT, Node4>) return MaxKeys_4;
        else if constexpr (std::is_same_v<NodeT, Node16>) return MaxKeys_16;
        else if constexpr (std::is_same_v<NodeT, Node32>) return MaxKeys_32;
        else return -1; // fallback
    }();
    
    static constexpr int KeyLen= []() {
        if constexpr (std::is_same_v<NodeT, Node8>) return 8;
        else if constexpr (std::is_same_v<NodeT, Node4>) return 4;
        else if constexpr (std::is_same_v<NodeT, Node16>) return 16;
        else if constexpr (std::is_same_v<NodeT, Node32>) return 32;
        else return -1; // fallback
    }();
    bool is_less_than(const char left[KeyLen], const char right[KeyLen]);
    struct Insert_Up_Data {
        char key[KeyLen] = {0};
        off_t left_child = 0;
        off_t right_child = 0;
    };
    BtreePlus();
    BtreePlus(File *file);
    
    //@brief insertes string a and corresponding value b
    void insert(std::string insert_string, off_t &record_location);

    //@brief deletes a key and its value
    void delete_key(std::string delete_string);


    LocationData<LeafNodeType> locate(std::string key);

    //@brief searches the index tree for a value returns offset of the record, if no is found, return 0;
    std::vector<off_t> search(std::string search_string);
    
    //@brief checks if the tree contains a certian key
    bool has_key(const std::string &key);

    //@brief prints current objects tree, assumes roo_node is defined and loaded 
    void print_tree();

    //@brief creates the first node and loads it to the cache.
    void init_root(off_t &location);

    //@brief returns LeafNode Type of the furthest left node
    LeafNodeT* find_leftmost_leaf();
private:
    //----------handle deletion----------------
    bool check_underflow(NodeT* node);
    void delete_index_in_node(int index, NodeT* node, int child_index);
    void leaf_merge(InternalNodeT* parent,NodeT* current, int child_index);
    void internal_underflow(NodeT* node);
    void merge_internal(InternalNodeT* node, InternalNodeT* parent, int child_index);
    bool attempt_borrow_internal(InternalNodeT* self, InternalNodeT* parent, int self_child_index);
    void borrow_left_internal(InternalNodeT *self,InternalNodeT* parent, InternalNodeT* left, int child_index);
    void borrow_right_internal(InternalNodeT *self,InternalNodeT* parent, InternalNodeT* right, int child_index);
    bool attempt_borrow(NodeT *current, InternalNodeT *parent, int currents_child_index);
    void borrow_left_leaf(NodeT* current, NodeT* left);
    void borrow_right_leaf(NodeT* current, NodeT* right);
    
    //----------handle insertion----------------
    void insert_up_into(Insert_Up_Data data,off_t node_location);
    void split_leaf(NodeT* node);
    void split_internal(NodeT* node);
    void insert_key_into_node(Insert_Up_Data data, NodeT* node);
    void push_into_internal(InternalNodeT* target, char* value);
    
    //----------utill functions----------------
    int find_child_index(InternalNodeT* parent, off_t child);
    off_t get_next_node_pointer(char* to_insert, InternalNodeT *node);
    int find_left_node_child_index(NodeT *node);
    void print_recursive(NodeT* node, int depth, std::ostream& out);
    int get_underflow_amount();
    int leaf_contains(NodeT* leaf, const std::string &key);
    off_t search_recursive(char* search_key, InternalNodeT* node);
};
using MyBtree32 = BtreePlus<Node32, LeafNode32, InternalNode32>;
using MyBtree16  = BtreePlus<Node16, LeafNode16,InternalNode16>;
using MyBtree8  = BtreePlus<Node8, LeafNode8,InternalNode8>;
using MyBtree4  = BtreePlus<Node4, LeafNode4,InternalNode4>;