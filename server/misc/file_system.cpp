#include "btree.hpp"
#include "file_system.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h> // For fallocatej
#include <unistd.h>


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


    
    if (open_file(file_fd) != 1)
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
        header_block_creation(file_fd);
    }
    else
    {
        header_pointer = 0;
        
        lseek(file_fd, 0, SEEK_SET);
        table_block_pointer = get_table_block(file_fd);

        lseek(file_fd, 8, SEEK_SET);
        root_node_pointer = get_root_node(file_fd);

        lseek(file_fd,16, SEEK_SET);
        root_data_pointer = get_root_data(file_fd);

        std::cout << table_block_pointer << " " << root_data_pointer;
    }




    
}

int File::add_data_to(char* data, int length, int block_id, off_t block_offset) {
    off_t offset = block_id * BLOCK_SIZE + block_offset;
    if (lseek(file_fd, offset, SEEK_SET) == -1) {
        perror("lseek failed");
        return -1;
    }

    ssize_t n = write(file_fd, data, length);
    if (n != length) {
        perror("write failed");
        return -1;
    }

    return 0;
}

off_t File::alloc_block() {
    off_t offset = lseek(file_fd, 0, SEEK_END);
    if (offset == -1) {
        perror("lseek failed");
        exit(1);
    }

    if (fallocate(file_fd, 0, offset, BLOCK_SIZE) == -1) {
        perror("fallocate failed");
        exit(1);
    }

    return offset;
}




void File::insert_data(BtreePlus &tree, std::string key, std::string value)
{
    Node *loaded_node = load_node(root_node_pointer); 
    off_t data = write_data(value.c_str(), value.length()); 

    tree.root_node = loaded_node;


    tree.insert(key, data);



}

void File::read_node_block(off_t block_pointer) {
    char data[sizeof(Node)];
    if (lseek(file_fd, block_pointer, SEEK_SET) == -1) {
        perror("lseek failed");
        exit(1);
    }

    ssize_t n = read(file_fd, data, sizeof(Node));
    if (n != sizeof(Node)) {
        perror("read failed");
        exit(1);
    }

    Node output;
    memcpy(&output, data, sizeof(Node));
}


void File::header_block_creation(int file_fd)
{
    //set table block pointer
    table_block_pointer = this->alloc_block();
    
    lseek(file_fd, 0, SEEK_SET);

    perror("first seek success");
    write(file_fd,&table_block_pointer, sizeof(table_block_pointer)); 


    //set root node pointer
    root_node_pointer = this->alloc_block();
    lseek(file_fd, 8, SEEK_SET);
    write(file_fd, &root_node_pointer,sizeof(root_node_pointer));
    
    init_root_node(file_fd);



    //set root data node pointer
    root_data_pointer = this->alloc_block();
    lseek(file_fd,16, SEEK_SET);
    write(file_fd, &root_data_pointer,sizeof(root_data_pointer));

    std::cout << root_data_pointer;
    off_t current_offset = lseek(file_fd, root_data_pointer, SEEK_SET);

    init_data_node(current_offset);


}
void File::update_root_pointer()
{
    //lseek(file_fd, header_pointer, SEEK_SET);
    off_t pos = lseek(file_fd, header_pointer, SEEK_SET);
    //write(&root_node_pointer, sizeof(root_node_pointer), 1, file_fd);
    ssize_t n = write(file_fd, &root_node_pointer, sizeof(root_node_pointer));
    if (n == -1) { perror("write"); }


    //fflush(file_fd); 
}



void File::update_node(Node *node, off_t node_location)
{

    lseek(file_fd, node_location, SEEK_SET);

    // write the actual Node struct, not the pointer
    if (write(file_fd,node, sizeof(internal_node)) != 1) {
        perror("write failed");
        exit(1);
    }


    std::cout << std::flush;
}

Node* File::load_node(off_t disk_offset) {

    lseek(file_fd, disk_offset, SEEK_SET);

    // Read base node to decide type
    Node temp;
    read(file_fd, &temp, sizeof(Node));

    Node* node;
    if (temp.is_leaf) {
        // Leaf node
        leaf_node* leaf = new leaf_node();
        lseek(file_fd, disk_offset, SEEK_SET);
        read(file_fd, leaf, sizeof(leaf_node));
        node = leaf;
    } else {
        // Internal node
        internal_node* internal = new internal_node();
        lseek(file_fd, disk_offset, SEEK_SET);
        read(file_fd, internal, sizeof(internal_node));
        node = internal;
    }


    return node;
}

void File::init_root_node(int file_fd)
{
    leaf_node node;
    node.disk_location = root_node_pointer;
    node.is_leaf = true;
    
    File::update_node(&node,node.disk_location);
}
off_t File::get_root_node(int file_fd)
{
    char buffer[8] = {0};
    if (read(file_fd, buffer, 8) != 8) {
        perror("read");
        return -1; // error
    }

    off_t output = 0;
    memcpy(&output, buffer, sizeof(output));

    return output;
}
int File::open_file(int file_fd)
{
    file_fd = open(file_name.c_str(), O_RDWR | O_CREAT, 0644);
if (file_fd == -1) {
    perror("open");
    return 1;
}

    if (!file_fd)
    {
        std::cerr << "couldnt open file_fd \n";
        return 1;
    }
    void* buf;
    if (posix_memalign(&buf, BLOCK_SIZE, BLOCK_SIZE) != 0) {
        perror("posix_memalign");
        return 1;
    }

    return 0;


}
Node File::cast_node(off_t node_offset)
{
    lseek(file_fd, node_offset, SEEK_SET);

    Node output;
    if (read(file_fd, &output, sizeof(Node)) != 1) {
        perror("Failed to read Node from file_fd");
        exit(1);
    }

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


void File::init_data_node(off_t location)
{
    //file_fd now points to byte 0 of the data seg
    Data_Node data;
    data.data_offset = location + sizeof(Data_Node);
    data.space_left = BLOCK_SIZE - sizeof(Data_Node);
    data.overflow = false;
    data.data_next = 0;


    //write(&data, sizeof(data), 1, file_fd);
    write(file_fd, &data, sizeof(data));




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

            lseek(file_fd,write_back_location, SEEK_SET);

            init_data_node(write_back_location);

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
        lseek(file_fd, node.data_offset, SEEK_SET);
        
        //write(data_to_insert,data_length,1,file_fd);
        write(file_fd, data_to_insert, data_length);


        to_return = node.data_offset;

        node.space_left -= data_length;
        node.data_offset = node.data_offset + data_length;
        

        lseek(file_fd, write_back_location, SEEK_SET);

        //write(&node, sizeof(Data_Node), 1, file_fd);
        write(file_fd, &node, sizeof(Data_Node));


    }

    
    return to_return;
}

Data_Node File::load_data_node(off_t location)
{
    lseek(file_fd, location, SEEK_SET);

    
    Data_Node temp_node;

    read(file_fd, &temp_node, sizeof(Data_Node));





    //todo: edge 
    return temp_node;
}

off_t File::get_root_data(int file_fd)
{
    char buffer[8] = {0};
    if (read(file_fd, buffer, 8) != 8) {
        perror("read");
        return -1; // error
    }

    off_t output = 0;
    memcpy(&output, buffer, sizeof(output));

    return output;

}


off_t File::get_table_block(int file_fd)
{
    char buffer[8] = {0};
    if (read(file_fd, buffer, 8) != 8) {
        perror("read");
        return -1; // error
    }

    off_t output = 0;
    memcpy(&output, buffer, sizeof(output));

    return output;
}


void File::init_table_block(int file_fd)
{

}

Table File::load_table()
{
    //lseek(file_fd, table_block_pointer, SEEK_SET);
    lseek(file_fd, table_block_pointer, SEEK_SET);
    std::byte buffer[256];
    ///read(buffer, 256, 1, file_fd);
    read(file_fd, buffer,256);

    Table output_table(buffer, 256);
    return output_table;
}

off_t File::insert_table(Table *table)
{
    std::cout << "inserted: ";
    //lseek(file_fd, table_block_pointer, SEEK_SET);
    lseek(file_fd, table_block_pointer, SEEK_SET);
    
    std::vector<std::byte> casted_table = cast_to_bytes(table);
    auto *table_data = casted_table.data();

    //write(table_data, casted_table.size(), 1, file_fd);
    write(file_fd, table_data, casted_table.size());

    Table test = load_table();
}






