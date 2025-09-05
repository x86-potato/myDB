#include "file_system.hpp"
#include "btree.hpp"

#include <iostream>
#include <cassert>
#include <cstring>
#include <string>



#define PAGE_SIZE 256 //256 bytes
#define MAX_KEYS 4



bool is_less_than(const char left[32], const char right[32])
{
    return std::strncmp(left, right, 32) < 0;
}




BtreePlus::BtreePlus(File &file) : file(file)
{ 
            
}
off_t BtreePlus::get_next_node_pointer(char* to_insert, internal_node *node)       //TODO: IMPROVE MEMORY SAFETY
{
    for (uint16_t i = 0; i < node->current_key_count; i++)
    {
        if(is_less_than(to_insert,node->keys[i]))
        {
            return node->children[i];        //case if found key bigger, then fitting spot

        }
    }
    //case if no key bigger is found, then biggest key, return very last child
    return node->children[node->current_key_count];
    
}
int BtreePlus::find_left_node_child_index(Node *node)
{
    Node* loaded_parent = file.load_node(node->parent);
    internal_node *parent = static_cast<internal_node*>(loaded_parent); 

    for (int i = 0; i < MAX_KEYS +1; i++)
    {
        if(parent->children[i] == node->disk_location)
        {
            return i;
        }
    }

    return -1;
    
}

void BtreePlus::insert(std::string insert_string)
{
    //stirng to c_str
    char buffer[32];
    std::strncpy(buffer, insert_string.c_str(), 32);
    char to_insert[32];
    std::strncpy(to_insert, buffer, 32);

    
    
    Node* cursor = root_node;               //TODO: ERROR

    if(!root_node)                  //case first node   !!never hits
    {

        perror("never hits");
        Node *new_node = new leaf_node(); 
        root_node = new_node; 
        std::strncpy(root_node->keys[0],to_insert,32);
        root_node->current_key_count = 1;   

        file.update_node(new_node, new_node->disk_location); 

        return;
    }
    

    while(!cursor->is_leaf)
    {
        internal_node *cursor_cast = static_cast<internal_node*>(cursor);
        cursor = file.load_node(get_next_node_pointer(to_insert,cursor_cast));
    }


    //insert into current node
    insert_up_data data = {};
    strncpy(data.key,to_insert,32);
    data.left_child = 0;
    data.right_child = 0;
    insert_key_into_node(data,cursor);

    file.update_node(cursor, cursor->disk_location);


    if(cursor->current_key_count == MAX_KEYS)
    {
        perror("leaf split called");
        split_leaf(cursor);
    }


}

void BtreePlus::insert_up_into(insert_up_data data,off_t node_location)
{
    Node *node = file.load_node(node_location);
    insert_key_into_node(data,node);


    if(node->current_key_count == MAX_KEYS)
    {
        split_internal(node);
    }
    file.update_node(node,node->disk_location);

}

void BtreePlus::split_leaf(Node* node)
{
    Node* right_node = new leaf_node();
    off_t right_node_location = file.alloc_block();
    std::cout << "alloced block : " << right_node_location;
    right_node->disk_location = right_node_location; 

    assert(node->is_leaf);
    leaf_node* node_cast = static_cast<leaf_node*>(node);
    leaf_node* right_node_cast = static_cast<leaf_node*>(right_node);
    right_node_cast->next_leaf = node_cast->next_leaf;
    node_cast->next_leaf = right_node_cast->disk_location;
    


    int middle_index = MAX_KEYS/2;
    char middle_key[32];  // ACTUAL ARRAY to store the key value
    std::strncpy(middle_key, node->keys[middle_index], 32);  // COPY the value

    right_node->parent = node->parent;

    char temp_keys[MAX_KEYS][32];
    std::memcpy(temp_keys, node->keys, sizeof(node->keys));

    for (int i = middle_index; i < MAX_KEYS; i++) {
        std::fill(std::begin(node->keys[i]), std::end(node->keys[i]), 0);
    }


    for (int i = middle_index; i < MAX_KEYS; i++) {
        std::strncpy(right_node->keys[i - middle_index], temp_keys[i], 32);
    }   

    node->current_key_count = middle_index;
    right_node->current_key_count = MAX_KEYS - middle_index;

    if (node->parent == 0)
    {
        Node* new_parent = new internal_node();
        new_parent->disk_location = file.alloc_block();
        internal_node* new_parent_cast = static_cast<internal_node*>(new_parent);
        new_parent->is_leaf = false;
        new_parent_cast->children[0] = node->disk_location;
        new_parent_cast->children[1] = right_node->disk_location;

        node->parent = new_parent->disk_location;
        right_node->parent = new_parent->disk_location;

        root_node = new_parent;
        file.root_node_pointer = new_parent->disk_location;

        std::strncpy(new_parent->keys[0], middle_key, 32);
        new_parent->current_key_count = 1;            

        file.update_node(right_node,right_node->disk_location);
        file.update_node(node,node->disk_location);
        file.update_node(new_parent,new_parent->disk_location);
    }
    else
    {
        insert_up_data data = {};

        strncpy(data.key,middle_key,32);
        data.left_child = node->disk_location;
        data.right_child = right_node->disk_location;

        file.update_node(right_node,right_node->disk_location);
        file.update_node(node,node->disk_location);
        

        insert_up_into(data,node->parent);


    }
}

void BtreePlus::split_internal(Node* node)
{
    Node* right_node = new internal_node();
    off_t right_node_location = file.alloc_block();
    right_node->disk_location = right_node_location;

    right_node->is_leaf = false;
    right_node->parent = node->parent;

    int middle_index = MAX_KEYS/2;
    char middle_key[32];  // ACTUAL ARRAY to store the key value
    std::strncpy(middle_key, node->keys[middle_index], 32);  // COPY the value

    char temp_keys[MAX_KEYS][32];
    std::memcpy(temp_keys, node->keys, sizeof(node->keys)); // copy all keys

    // Zero out the right half of the left node
    for (int i = middle_index; i < MAX_KEYS; i++) {
        std::fill(std::begin(node->keys[i]), std::end(node->keys[i]), 0);
    }

    // Copy the upper half into the right node
    internal_node* right_leaf = static_cast<internal_node*>(right_node);
    for (int i = middle_index+1; i < MAX_KEYS; i++) {
        std::memcpy(right_leaf->keys[i - middle_index-1], temp_keys[i], 32);
    }


    node->current_key_count = middle_index;
    right_node->current_key_count = MAX_KEYS - middle_index - 1;

    // handle children pointers
    internal_node* node_cast = static_cast<internal_node*>(node);
    internal_node* right_cast = static_cast<internal_node*>(right_node);

    // left node keeps first (middle_index + 1) children
    // right node gets remaining children
    
    for (int i = middle_index + 1; i <= MAX_KEYS; i++) {
        right_cast->children[i - (middle_index + 1)] = node_cast->children[i];

        
        if (node_cast->children[i]) {  // FIXED: Update parent pointer
            Node* child = file.load_node(node_cast->children[i]);
            child->parent = right_node->disk_location;
            file.update_node(child,child->disk_location);
        }
        node_cast->children[i] = 0;
    }

    if (node->parent != 0)
    {
        insert_up_data data = {};
        std::strncpy(data.key, middle_key, 32);
        std::cout<<"middile key: " << middle_key;
        data.left_child = node->disk_location;
        data.right_child = right_node->disk_location;

        file.update_node(right_node,right_node_location);
        file.update_node(node,node->disk_location);
        insert_up_into(data,node->parent);
    
    }
    else
    {
        Node* new_parent = new internal_node();
        off_t new_parent_location = file.alloc_block();
        new_parent->disk_location = new_parent_location;
        root_node = new_parent;
        file.root_node_pointer = new_parent_location;


        internal_node* new_parent_cast = static_cast<internal_node*>(new_parent);

        new_parent_cast->children[1] = right_node->disk_location;
        new_parent_cast->children[0] = node->disk_location;
        new_parent->is_leaf = false;
        new_parent->current_key_count = 1;

        node->parent = new_parent->disk_location;
        right_node->parent = new_parent->disk_location;
        

        std::strncpy(new_parent->keys[0], middle_key, 32);

        file.update_node(new_parent, new_parent_location);
        file.update_node(node, node->disk_location);
        file.update_node(right_node, right_node->disk_location);
    }

}


leaf_node* BtreePlus::find_leftmost_leaf(Node* root) {
    if (!root) return nullptr;
    Node* curr = root;

    while (!curr->is_leaf) {
        internal_node* in = static_cast<internal_node*>(curr);

        curr = file.load_node(in->children[0]); // go all the way lef
    }
    return static_cast<leaf_node*>(curr);
}


void BtreePlus::insert_key_into_node(insert_up_data data, Node* node)
{
    int insert_positon = 0;

    while (insert_positon < node->current_key_count && is_less_than(node->keys[insert_positon],data.key)) {
        insert_positon++;
    }
    
    // Shift elements to the right to make space
    for (int i = node->current_key_count; i > insert_positon; i--) {
        std::strncpy(node->keys[i], node->keys[i-1], 32);
    }
    //if is an internal node, shift children right too
    if(!node->is_leaf)
    {               
        internal_node *node_cast = static_cast<internal_node*>(node);                                                                    //insert 3, z, i    //position = 2
        for (int i = node->current_key_count+1; i > insert_positon; i--) 
        {                                                                                     //keys[1,2,3,4,0,0] //children[x,y,z,t,0,0] // x y z   t 0 i = 3
            node_cast->children[i] = node_cast->children[i - 1];
        }
        node_cast->children[insert_positon + 1] = data.right_child;
    } 
    // Insert the new key
    std::strncpy(node->keys[insert_positon], data.key, 32);
    
    node->current_key_count++;
}




int func(BtreePlus &myTree, File &file)
{



    //myTree.print_tree(myTree.root_node,10);
    leaf_node* leaf =  myTree.find_leftmost_leaf(myTree.root_node);

    while (leaf)
    {
        std::cout << "\n leaves: ";
        for(auto key : leaf->keys) 
        {
            std::cout << key << " ";
        }
        leaf = static_cast<leaf_node*>(file.load_node(leaf->next_leaf));
    }
    std::cout << "leaves done \n";
    if (myTree.root_node && !myTree.root_node->is_leaf) 
    {
        internal_node* in = static_cast<internal_node*>(myTree.root_node);
        std::cout << "\nRoot keys: ";
        for (int i = 0; i < in->current_key_count; i++) {
            std::cout.write(in->keys[i], 32) << " ";
        }

        std::cout << "\nChildren: ";
        for (int i = 0; i <= in->current_key_count; i++) {  // note: #children = keys+1
            if (in->children[i]) {
                Node *child = file.load_node(in->children[i]);
                std::cout << "(" << i << ")->" << child->keys[0] << " ";
            }
        }
        std::cout << "\n";
    }
        
    return 0;
}