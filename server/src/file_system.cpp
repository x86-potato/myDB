#include "btree.hpp"
#include "file_system.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h> // For fallocatej



#define BLOCK_SIZE 4096



std::string file_name = "database.db"; 
int index_block_count;
int data_blocks_count;
status file_status = status::EMPTY; 
off_t header_pointer;
off_t root_node_pointer;


File::File()
{
    index_block_count = 0;
    data_blocks_count = 0;

    struct stat st;
    stat(file_name.c_str(), &st);


    FILE* file = fopen(file_name.c_str(),"r+b");
    if (open_file(file) != 1)
    {
        if(st.st_size > 0)
        {
            file_status = status::READY;
        }
        else
        {
            file_status = status::EMPTY;
        } 
    }

    if (file_status == EMPTY)
    {
        header_pointer = alloc_block();
        header_block_creation(file);
    }
    else
    {
        header_pointer = 0;
        root_node_pointer = get_root_node(file);
        fseek(file,sizeof(root_node_pointer), SEEK_SET);
        root_data_pointer = get_root_data(file);

    }



    fclose(file);

    
}

int File::add_data_to(char* data, int length, int block_id, off_t block_offset)
{
    FILE* file = fopen(file_name.c_str(),"r+b");

    fseek(file, block_id * BLOCK_SIZE + block_offset, SEEK_SET);

    fwrite(data, sizeof(data[0]), length, file);

    fclose(file);

    return 0;
}


off_t File::alloc_block()
{

    FILE* file = fopen(file_name.c_str(),"r+b");

    fseek(file, 0,SEEK_END);

    int fd = fileno(file);
    fallocate(fd, 0, file->_offset, BLOCK_SIZE);

    


    return file->_offset;

}



void File::insert_data(BtreePlus &tree, std::string key, std::string value)
{
    Node *loaded_node = load_node(root_node_pointer); 
    off_t data = write_data(value.c_str(), value.length()); 

    tree.root_node = loaded_node;


    tree.insert(key, data);



}

void File::read_node_block(off_t block_pointer)
{

    FILE* file = fopen(file_name.c_str(),"r+b");
    fseek(file,block_pointer,SEEK_SET);
    char data[sizeof(Node)];

    fread(data,sizeof(Node),1,file);
    
    Node output;
    memcpy(&output, data, sizeof(Node));


    fclose(file);
}


void File::header_block_creation(FILE* file)
{
    //set root node pointer
    root_node_pointer = File::alloc_block();
    fseek(file, header_pointer, SEEK_SET);
    fwrite(&root_node_pointer,sizeof(root_node_pointer),1,file);
    
    init_root_node(file);


    //set root data node pointer
    root_data_pointer = this->alloc_block();
    fseek(file,sizeof(root_node_pointer), SEEK_SET);
    fwrite(&root_data_pointer,sizeof(root_data_pointer), 1, file);

    fseek(file, root_data_pointer, SEEK_SET);

    init_data_node(file, ftell(file));


}
void File::update_root_pointer()
{
    FILE* file = fopen(file_name.c_str(), "r+b");
    fseek(file, header_pointer, SEEK_SET);
    fwrite(&root_node_pointer, sizeof(root_node_pointer), 1, file);

    fclose(file); 
}



void File::update_node(Node *node, off_t node_location)
{
    FILE* file = fopen(file_name.c_str(), "r+b");
    if (!file) {
        perror("fopen failed");
        exit(1);
    }

    fseek(file, node_location, SEEK_SET);

    // write the actual Node struct, not the pointer
    if (fwrite(node, sizeof(internal_node), 1, file) != 1) {
        perror("fwrite failed");
        fclose(file);
        exit(1);
    }


    std::cout << std::flush;
    fclose(file);
}

Node* File::load_node(off_t disk_offset) {
    FILE* file = fopen(file_name.c_str(),"r+b");
    if (!file) return nullptr;

    fseek(file, disk_offset, SEEK_SET);

    // Read base node to decide type
    Node temp;
    fread(&temp, sizeof(Node), 1, file);

    Node* node;
    if (temp.is_leaf) {
        // Leaf node
        leaf_node* leaf = new leaf_node();
        fseek(file, disk_offset, SEEK_SET);
        fread(leaf, sizeof(leaf_node), 1, file);
        node = leaf;
    } else {
        // Internal node
        internal_node* internal = new internal_node();
        fseek(file, disk_offset, SEEK_SET);
        fread(internal, sizeof(internal_node), 1, file);
        node = internal;
    }


    fclose(file);
    return node;
}

void File::init_root_node(FILE* file)
{
    leaf_node node;
    node.disk_location = root_node_pointer;
    node.is_leaf = true;
    
    File::update_node(&node,node.disk_location);
}
off_t File::get_root_node(FILE* file)
{
    char buffer[8] = {0};
    if (fread(buffer, 1, 8, file) != 8) {
        perror("fread");
        return -1; // error
    }

    off_t output = 0;
    memcpy(&output, buffer, sizeof(output));

    return output;
}
int File::open_file(FILE* file)
{
    if (!file)
    {
        std::cerr << "couldnt open file \n";
        return 1;
    }

    return 0;


}
Node File::cast_node(off_t node_offset)
{
    FILE* file = fopen(file_name.c_str(), "rb");
    if (!file) {
        perror("Failed to open file");
        exit(1);
    }

    fseek(file, node_offset, SEEK_SET);

    Node output;
    if (fread(&output, sizeof(Node), 1, file) != 1) {
        perror("Failed to read Node from file");
        fclose(file);
        exit(1);
    }

    fclose(file);
    return output;
}


void File::print_leaves(off_t disk_node_offset) {
    if (root_node_pointer == 0) {
        std::cout << "Tree is empty\n";
        return;
    }

    //we will call this recursively on each nodes leaves
    Node *node = load_node(disk_node_offset);
    std::string type;

    type = "\ninternal: ";
    if(node->is_leaf)
        type = "\nleaf: ";
    
    std::cout << type << disk_node_offset << " keys: \n";
    for (int i = 0; i < node->current_key_count; i++)
    {
        std::cout << node->keys[i] << " ";
    }
    std::cout << std::endl;
    if (!node->is_leaf)    
    {
        internal_node* node_cast = static_cast<internal_node*>(node);
        for (int i = 0; i < MAX_KEYS; i++)
        {
            if(node_cast->children[i] != 0)
            {
                print_leaves(node_cast->children[i]);
            }
        }
        
    }
    else 
    {
        leaf_node* node_cast = static_cast<leaf_node*>(node);
        for (int i = 0; i < MAX_KEYS; i++)
        {
            if(node_cast->values[i] != 0)
            {
                std::cout<< node_cast->values[i] << " ";
            }
        }
    }
}


void File::init_data_node(FILE* file, off_t location)
{
    //file now points to byte 0 of the data seg
    Data_Node data;
    data.data_offset = location + sizeof(Data_Node);
    data.space_left = BLOCK_SIZE - sizeof(Data_Node);
    data.overflow = false;
    data.data_next = 0;


    fwrite(&data, sizeof(data), 1, file);
    fflush(file);




}    

off_t File::write_data(const char *data_to_insert, u_int16_t data_length)
{
    Data_Node node = load_data_node(root_data_pointer);
    off_t write_back_location = root_data_pointer;
    off_t to_return;

    while(node.space_left < data_length)        //search for data node that has space
    {
        if(node.data_next == 0)
        {
            off_t write_back_location = alloc_block();

            FILE* file = fopen(file_name.c_str(), "r+b");    
            fseek(file,write_back_location, SEEK_SET);

            init_data_node(file,write_back_location);

            node.data_next = write_back_location;

            node = load_data_node(write_back_location);
        
            break;
        }
        else
        {
            node = load_data_node(node.data_next);
            write_back_location = node.data_next;

        }
        
    }


    if (node.space_left > data_length)
    {
        FILE* file = fopen(file_name.c_str(), "r+b");    
        fseek(file, node.data_offset, SEEK_SET);
        
        fwrite(data_to_insert,data_length,1,file);
        fflush(file);


        to_return = node.data_offset;

        node.space_left -= data_length;
        node.data_offset = node.data_offset + data_length;
        

        fseek(file, write_back_location, SEEK_SET);

        fwrite(&node, sizeof(Data_Node), 1, file);

        fflush(file);

    }

    
    return to_return;
}

Data_Node File::load_data_node(off_t location)
{
    FILE* file = fopen(file_name.c_str(), "rb");    
    fseek(file, location, SEEK_SET);

    
    Data_Node temp_node;

    fread(&temp_node, sizeof(Data_Node), 1, file);





    //todo: edge 
    return temp_node;
}

off_t File::get_root_data(FILE* file)
{
    char buffer[8] = {0};
    if (fread(buffer, 1, 8, file) != 8) {
        perror("fread");
        return -1; // error
    }

    off_t output = 0;
    memcpy(&output, buffer, sizeof(output));

    return output;
}