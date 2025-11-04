#include "config.h"
#include "file.hpp"
#include "btree.hpp"

File::File()
{
    index_block_count = 0;
    data_blocks_count = 0;


    struct stat st;
    stat(file_name.c_str(), &st);


    
    if (open_file() != 1)
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

    cache.filefd = file_fd;

    if (file_status == EMPTY)
    {
        
        header_pointer = alloc_block();
        std::cout << header_pointer << "\n";

        header_block_creation();

        
        
        std::cout << "table block,root node, root data: "<< table_block_pointer << " "<< root_node_pointer  << " " << free_data_pointer << std::endl;

        //primary_table = load_table();
    }
    else
    {
        header_pointer = 0;
        
        table_block_pointer = get_table_block();
        root_node_pointer = get_root_node();
        free_data_pointer = get_data();

        primary_table = load_table();
        primary_table.table_print();
        std::cout << "table block,root node, root data: "<< table_block_pointer << " "<< root_node_pointer  << " " << free_data_pointer << std::endl;

    }
}

//@brief called upon 
template<typename MyBtree, typename NodeT, typename InternalNodeT, typename LeafNodeT>    
void File::insert_data(std::string key,Record &record, MyBtree index_tree)
{
    NodeT *loaded_node = load_node<NodeT,InternalNodeT, LeafNodeT>(root_node_pointer); 
    if(loaded_node->disk_location == 0) index_tree.init_root();

    index_tree.root_node = loaded_node;

    InsertResult result = index_tree.insert(key, record);

    if(result == Failed) {std::cout << "\nInsertion of " << key << " failed, duplicate key\n"; return; }





}

template<typename MyBtree, typename NodeT, typename InternalNodeT, typename LeafNodeT>    
Record File::find(std::string key, MyBtree index_tree)
{
    off_t location = index_tree.search(key);

    Record temp = get_record(location);

    return temp;
}

void File::update_root_pointer()    
{
    Page* page = cache.read_block(header_pointer); // Use the correct header pointer!

    cache.write_to_page(page, HEADER_ROOT_NODE_LOCATION, &root_node_pointer, sizeof(root_node_pointer),header_pointer);

}

off_t File::alloc_block()
{
    // Find the current end of file
    off_t offset = lseek(file_fd, 0, SEEK_END);
    if (offset == -1) {
        perror("lseek failed");
        exit(1);
    }

    // Allocate a new block at the end
    if (fallocate(file_fd, 0, offset, BLOCK_SIZE) == -1) {
        perror("fallocate failed");
        exit(1);
    }

    return offset;
}

template<typename NodeT>
void File::update_node(NodeT *node, off_t node_location)
{
    Page* page = cache.read_block(node_location);

    cache.write_to_page(page,0,node,BLOCK_SIZE, node_location);

}
template<typename LeafNodeT>
void File::update_leafnode(LeafNodeT *node, off_t node_location)
{
    Page* page = cache.read_block(node_location);

    cache.write_to_page(page,0,node,BLOCK_SIZE, node_location);

}

template<typename NodeT, typename InternalNodeT, typename LeafNodeT>
NodeT* File::load_node(off_t disk_offset) {
    Page* page = cache.read_block(disk_offset);

    NodeT* node = reinterpret_cast<NodeT*>(page);

    return node;
}

template<typename BtreeT>
void File::print_leaves(off_t disk_node_offset)
{
    using NodeT = typename BtreeT::NodeType;
    using LeafNodeT = typename BtreeT::LeafNodeType;
    using InternalNodeT = typename BtreeT::InternalNodeType;


    if (root_node_pointer == 0) {
        std::cout << "Tree is empty\n";
        return;
    }

    //we will call this recursively on each nodes leaves
    NodeT *node = load_node<NodeT, LeafNodeT, InternalNodeT>(disk_node_offset);
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
        InternalNodeT* node_cast = static_cast<InternalNodeT*>(node);
        for (int i = 0; i < node->current_key_count+1; i++)
        {
            if(node_cast->children[i] != 0)
            {
                print_leaves<BtreeT>(node_cast->children[i]);
            }
        }
        
    }
    else 
    {
        LeafNodeT* node_cast = static_cast<LeafNodeT*>(node);
        for (int i = 0; i < node->current_key_count; i++)
        {
            if(node_cast->values[i] != 0)
            {
                std::cout<< node_cast->values[i] << " ";
            }
        }
    }
}


off_t File::insert_table(Table *table)
{
    Page *page = cache.read_block(table_block_pointer);
    
    std::vector<std::byte> casted_table = cast_to_bytes(table);
    auto *table_data = casted_table.data();

        


    memcpy(page,table_data,casted_table.size());

    cache.write_block(table_block_pointer);

    primary_table = load_table();

    return 0;
}



Table File::load_table()
{
    Page *page = cache.read_block(table_block_pointer);



    Table output_table(page->buffer, 256);
    return output_table;
}



void File::header_block_creation()
{

    Page* header_page = cache.read_block(header_pointer);

    std::byte temp_header[24];

    //set table block pointer
    table_block_pointer = this->alloc_block();
    memcpy(&temp_header[0],&table_block_pointer,8);

    //init_table_block();
    //set root node pointer
    root_node_pointer = this->alloc_block();
    memcpy(&temp_header[8],&root_node_pointer,8);
    
    //init_root_node<Node8>();
    
    //set root data node pointer
    free_data_pointer = this->alloc_block();
    memcpy(&temp_header[16],&free_data_pointer,8);


    init_data_node(free_data_pointer);

    cache.write_to_page(header_page,0,temp_header,24, 0);


    //cache.flush_cache();
}
//void File::init_table_block();
template<typename NodeT>
void File::init_root_node()
{
    NodeT node;
    node.disk_location = root_node_pointer;
    node.is_leaf = true;
    
    File::update_node(&node,node.disk_location);

}
void File::init_data_node(off_t location)
{
    Data_Node data;

    data.overflow = false;
    data.next_free_block = 0;

    Page* page = cache.read_block(location);

    memcpy(page->buffer, &data,sizeof(data));

    cache.write_block(location);
}    

off_t File::get_table_block()   //FIX
{
    Page* page = cache.read_block(header_pointer);


    off_t output = 0;
    memcpy(&output, page, 8);

    return output;
}
off_t File::get_root_node()
{
    Page* page = cache.read_block(0);

    off_t output = 0;

    //std::cout.write(reinterpret_cast<char*>(buffer),8); std::cout << std::flush;
    memcpy(&output, &page->buffer[8], 8);

    return output;
}
off_t File::get_data()
{
    Page* page = cache.read_block(0);

    off_t output = 0;

    //std::cout.write(reinterpret_cast<char*>(buffer),8); std::cout << std::flush;
    memcpy(&output, &page->buffer[HEADER_ROOT_DATA_LOCATION], 8);

    return output;

}

int File::open_file()
{
    int fd = open(file_name.c_str(), O_RDWR | O_CREAT, 0644);

    off_t offset = lseek(fd, 0, SEEK_END);
    if (offset == -1) {
        perror("lseek failed");
        exit(1);
    }

    
    close(fd);

    // Reopen with O_DIRECT for direct I/O reads/writes
    file_fd = open(file_name.c_str(), O_RDWR | O_DIRECT);

    if (file_fd == -1) {
        perror("open");
        return 1;
    }

    if (!file_fd)
    {
        std::cerr << "couldnt open file_fd \n";
        return 1;
    }
    
    

    return 0;


}
// @brief returns record located at an offset
Record File::get_record(off_t record_location)
{
    off_t page_offset = (record_location/BLOCK_SIZE) * BLOCK_SIZE;
    off_t record_offset = record_location % BLOCK_SIZE;


    Page *page = cache.read_block(page_offset);

    Record record(page->buffer + record_offset, primary_table);

    std::cout << "\nFound: " << record.str;

    return record;

} 

// @brief returns location of the new record
off_t File::write_record(Record &record)
{
    Page* page = cache.read_block(free_data_pointer);
    Data_Node *node;
    node = reinterpret_cast<Data_Node*>(page);

    off_t record_location = 0;
    int space_left = 4072 - node->fill_ptr;

    

    if(space_left > record.length)
    {

        record_location = free_data_pointer + (24 + node->fill_ptr);

        memcpy(node->padding + node->fill_ptr, record.str.data(), record.length);
        //cache.write_to_page(page, 24 + node->fill_ptr,record.str.data(), record.length, free_data_pointer);
        node->fill_ptr = node->fill_ptr + record.length;
        cache.write_to_page(page, 0,node, BLOCK_SIZE, free_data_pointer);


    }
    else
    {
        //handle overflow and new nodes

        free_data_pointer = alloc_block();
        Page *new_page = cache.read_block(free_data_pointer);

        Data_Node *node = load_data_node(free_data_pointer);
        init_data_node(free_data_pointer);


        record_location = free_data_pointer + (24 + node->fill_ptr);

        memcpy(node->padding + node->fill_ptr, record.str.data(), record.length);

        node->fill_ptr += record.length;


        cache.write_to_page(new_page, 0,node, BLOCK_SIZE, free_data_pointer);

    }
    
    
    return record_location;
    
}





Data_Node *File::load_data_node(off_t location)
{
    Page* page = cache.read_block(location);

    
    Data_Node *temp_node;


    temp_node = reinterpret_cast<Data_Node*>(page);




    //todo: edge 
    return temp_node;
}

template Node4* File::load_node<Node4, InternalNode4, LeafNode4>(off_t);
template void File::update_node<Node4>(Node4*, off_t);
template void File::update_node<InternalNode4>(InternalNode4*, off_t);

template Node8* File::load_node<Node8, InternalNode8, LeafNode8>(off_t);
template void File::update_node<Node8>(Node8*, off_t);
template void File::update_node<InternalNode8>(InternalNode8*, off_t);

template Node32* File::load_node<Node32, InternalNode32, LeafNode32>(off_t);
template void File::update_node<Node32>(Node32*, off_t);
template void File::update_node<InternalNode32>(InternalNode32*, off_t);


template void File::insert_data<MyBtree32, Node32, LeafNode32, InternalNode32>(std::string, Record&, MyBtree32);
template void File::insert_data<MyBtree8, Node8, LeafNode8, InternalNode8>(std::string, Record&, MyBtree8);
template void File::insert_data<MyBtree4, Node4, LeafNode4, InternalNode4>(std::string, Record&, MyBtree4);

template Record File::find<MyBtree32, Node32, InternalNode32,LeafNode32>(std::string, MyBtree32);
template Record File::find<MyBtree8, Node8, InternalNode8,LeafNode8>(std::string, MyBtree8);
template Record File::find<MyBtree4, Node4, InternalNode4, LeafNode4>(std::string, MyBtree4);



