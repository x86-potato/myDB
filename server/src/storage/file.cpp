#include "file.hpp"
#include "../core/database.hpp"

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

        header_block_creation();

        
        
        std::cout << "table block,root node, root data: "<< table_block_pointer << " " << free_data_pointer << std::endl;

        //primary_table = load_table();
    }
    else
    {
        header_pointer = 0;
        
        table_block_pointer = get_table_block();
        free_data_pointer = get_data();

        std::cout << "table block,root node, root data: "<< table_block_pointer   << " " << free_data_pointer << std::endl;

    }
}

//@brief called upon 
template<typename MyBtree, typename NodeT, typename InternalNodeT, typename LeafNodeT>    
void File::insert_data(std::string key,Record &record, MyBtree index_tree)
{
    //handle index insertions



}

template <typename MyBtree>
off_t File::insert_primary_index(std::string key,Record &record, MyBtree &tree)
{
    
//     using NodeT = typename MyBtree::NodeType;
//     //NodeT *loaded_node = load_node<NodeT>(primary_table.columns[0].indexLocation); 
//     //if(loaded_node->disk_location == 0) tree.init_root(primary_table.columns[0].indexLocation);

//     tree.root_node = loaded_node;
//     //tree.tree_root = primary_table.columns[0].indexLocation;
//     InsertResult result;

//     off_t record_location = 0;

//     result = tree.insert(key, record, record_location);



//   //  if(result == Failed) {std::cout << "\nInsertion of " << key << " failed, duplicate key\n"; return 0; }

//     //if(primary_table.columns[0].indexLocation != tree.tree_root)
//    // {
//    //     primary_table.columns[0].indexLocation = tree.tree_root;
//     //    update_root_pointer(0, primary_table.columns[0].indexLocation);
//     //}


//     return record_location;
    return 0;
}

template <typename PrimaryTree>
void File::parse_primary_tree(PrimaryTree &tree)
{
    
//     using PrimaryNodeT = typename PrimaryTree::NodeType;
//     using PrimaryLeafNodeT = typename PrimaryTree::LeafNodeType;
//     using PrimaryInternalNodeT = typename PrimaryTree::InternalNodeType;

//     PrimaryNodeT *node = load_node<PrimaryNodeT>(primary_table.columns[0].indexLocation);
//     std::string type;
//     PrimaryInternalNodeT* internal_node_cast;

//     while (!node->is_leaf)    
//     {
//         internal_node_cast = static_cast<PrimaryInternalNodeT*>(node);
//         node = load_node<PrimaryNodeT>(internal_node_cast->children[0]);
//     }

//     //here we reach the left most node
//     PrimaryLeafNodeT* node_cast = static_cast<PrimaryLeafNodeT*>(node);

//     //allocate the root


//     while (node_cast)
//     {
//         for (int i = 0; i < node_cast->current_key_count; i++)
//         {
//             //record_locations.push_back(node_cast->values[i]);
//         }
//         off_t next_leaf = node_cast->next_leaf;
//         if(next_leaf == 0)
//             node_cast = nullptr;
//         else
//         {
//             node = load_node<PrimaryNodeT>(next_leaf);
//             node_cast = static_cast<PrimaryLeafNodeT*>(node);
//         }
//     }

}



template<typename MyBtree32, typename MyBtree16, typename MyBtree8, typename MyBtree4>    
void File::generate_index(int columnIndex, 
    MyBtree32 &tree32, MyBtree16 &tree16, MyBtree8 &tree8, MyBtree4 &tree4)
{
    // primary_table.columns[columnIndex].indexLocation = alloc_block();
    // update_root_pointer(columnIndex, primary_table.columns[columnIndex].indexLocation);

    // //pick whcih tree to create
    // switch (primary_table.columns[columnIndex].type)
    // {
    //     case Type::INTEGER:{
    //         init_node<typename MyBtree4::NodeType>(primary_table.columns[columnIndex].indexLocation);
    //         break;
    //     } 
    //     case Type::CHAR32:{
    //         init_node<typename MyBtree32::NodeType>(primary_table.columns[columnIndex].indexLocation);
    //         break;
    //     }
    //     case Type::CHAR16:{
    //         init_node<typename MyBtree16::NodeType>(primary_table.columns[columnIndex].indexLocation);
    //         break;
    //     }
    //     case Type::CHAR8:{
    //         init_node<typename MyBtree8::NodeType>(primary_table.columns[columnIndex].indexLocation);
    //         break;
    //     }
    // }



}

template <typename MyBtree>
void File::insert_secondary_index(std::string key,Record &record, MyBtree &tree,
     off_t root, off_t record_location, int index)
{
    using NodeT = typename MyBtree::NodeType;
    NodeT *loaded_node = load_node<NodeT>(root); 
    if(loaded_node->disk_location == 0) tree.init_root(root);

    tree.root_node = loaded_node;
    tree.tree_root = root;
    InsertResult result;


    result = tree.insert(key, record, record_location);



    if(result == Failed) 
    {
        std::cout << "\nInsertion of " << key << " failed, duplicate key\n"; 
        return;
    }

    if(root != tree.tree_root)
    {
        root = tree.tree_root;
        update_root_pointer(index, root);
    }
}

template<typename MyBtree, typename NodeT, typename InternalNodeT, typename LeafNodeT>    
std::vector<Record> File::find(std::string key, MyBtree &index_tree, off_t root_location)
{
    std::vector<Record> output;
    index_tree.tree_root = root_location;
    std::vector<off_t> locations = index_tree.search(key);

    for (auto &location: locations)
        output.push_back(get_record(location));


    return output;
}

void File::update_root_pointer(int index, off_t value)    
{
    Page* page = cache.read_block(table_block_pointer); // Use the correct header pointer!

    //primary_table.columns[index].indexLocation = value;

    //std::vector<std::byte> casted = cast_to_bytes(&primary_table);

    //cache.write_to_page(page, 0, casted.data(), casted.size(), table_block_pointer);

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
void File::update_node(NodeT *node, off_t node_location, size_t size)
{
    Page* page = cache.read_block(node_location);

    int write_size = 0;

    

    cache.write_to_page(page,0,node, size, node_location);

}
template<typename LeafNodeT>
void File::update_leafnode(LeafNodeT *node, off_t node_location)
{
    Page* page = cache.read_block(node_location);

    cache.write_to_page(page,0,node,BLOCK_SIZE, node_location);

}

template<typename NodeT>
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

template <typename Node32,typename Node16, typename Node8, typename Node4>
off_t File::insert_table(Table table)
{
    Page *page = cache.read_block(table_block_pointer);
    int currentTableCount = 9;
    memcpy(&currentTableCount, page->buffer, 4);


    int writeOffset = 12;
    int read = 0;
    while(currentTableCount > 0 && read < 4084)
    {
        if(char(*(page->buffer + writeOffset)) == ';')
        {
            currentTableCount--;
        }
        writeOffset++;
        read++;
    }
    //writeOffset++;


    //init primary root

    off_t location = alloc_block();
    table.columns[0].indexLocation = location;
    switch (table.columns[0].type)
    {
        case Type::INTEGER:{
            init_node<Node4>(location);
            break;
        }
        case Type::CHAR32:{
            init_node<Node32>(location);
            break;
        }
        case Type::CHAR16:{
            init_node<Node8>(location);
            break;
        }
        case Type::CHAR8:{
            init_node<Node8>(location);
            break;
        }
    }

    
    std::vector<std::byte> casted_table = cast_to_bytes(&table);
    auto *table_data = casted_table.data();

    memcpy(&currentTableCount, page->buffer, 4);
    currentTableCount++;

    cache.write_to_page(page, 0, &currentTableCount, 4, table_block_pointer);
    cache.write_to_page(page, writeOffset, table_data, casted_table.size(), table_block_pointer);


    return 0;
}



std::vector<Table> File::load_table()
{
    Page* page = cache.read_block(table_block_pointer);
    std::vector<Table> fetched;

    int tableCount = 0;
    memcpy(&tableCount, page->buffer, 4);

    int nextTableStart = 12; // table data starts at offset 12

    while (fetched.size() < tableCount)
    {
        int tableLength = 0;

        // scan from nextTableStart
        while (char(page->buffer[nextTableStart + tableLength]) != ';')
        {
            tableLength++;
            assert(nextTableStart + tableLength < BLOCK_SIZE); // sanity check
        }

        // construct Table from correct start
        Table output_table(page->buffer + nextTableStart, tableLength);
        fetched.push_back(output_table);

        // advance nextTableStart past table + delimiter
        nextTableStart += tableLength + 1;
    }

    return fetched;
}




void File::header_block_creation()
{

    Page* header_page = cache.read_block(header_pointer);

    std::byte temp_header[16];

    //set table block pointer
    table_block_pointer = this->alloc_block();
    memcpy(&temp_header[0],&table_block_pointer,8);

    init_table_block(table_block_pointer);
    
    //set root data node pointer
    free_data_pointer = this->alloc_block();
    memcpy(&temp_header[8],&free_data_pointer,8);


    init_data_node(free_data_pointer);

    cache.write_to_page(header_page,0,temp_header,16, 0);


    cache.flush_cache();
}
//void File::init_table_block();
template<typename NodeT>
void File::init_node(off_t location)
{
    NodeT node;
    node.disk_location = 0;
    node.is_leaf = true;
    
    File::update_node(&node, location, sizeof(NodeT));

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

void File::init_table_block(off_t location)
{
    Page* page = cache.read_block(location);
    //first 4 bytes int
    //next 8 byts overflow

    char temp[12] = {0};
    int table_count = 0;
    off_t overflow = 0;

    memccpy(temp, &table_count, 1, sizeof(table_count));
    memccpy(temp + 4, &overflow, 1, sizeof(overflow));

    cache.write_to_page(page, 0, temp, 12, table_block_pointer);

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

    Record record(page->buffer + record_offset, database->tableMap.at(0));

   // std::cout << "\nFound: " << record.str;

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

        std::cout << "new block :" << free_data_pointer << "\n";

        Page *header_page = cache.read_block(0);

        cache.write_to_page(header_page, HEADER_ROOT_DATA_LOCATION, &free_data_pointer, 8, 0);

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

template off_t File::insert_table<Node32, Node16, Node8, Node4>(Table);
template void File::generate_index<MyBtree32, MyBtree16, MyBtree8, MyBtree4>(int, MyBtree32&,MyBtree16&,MyBtree8&,MyBtree4&);

template Node4* File::load_node<Node4>(off_t);
template void File::update_node<Node4>(Node4*, off_t, size_t);
template void File::update_node<InternalNode4>(InternalNode4*, off_t, size_t);

template off_t File::insert_primary_index<MyBtree4>(std::string,Record&, MyBtree4&);
template void File::insert_secondary_index<MyBtree4>(std::string,Record&, MyBtree4&,off_t, off_t, int);
template void File::parse_primary_tree<MyBtree4>(MyBtree4&);




template Node8* File::load_node<Node8>(off_t);
template void File::update_node<Node8>(Node8*, off_t, size_t);
template void File::update_node<InternalNode8>(InternalNode8*, off_t, size_t);

template off_t File::insert_primary_index<MyBtree8>(std::string,Record&, MyBtree8&);
template void File::insert_secondary_index<MyBtree8>(std::string,Record&, MyBtree8&,off_t, off_t, int);
template void File::parse_primary_tree<MyBtree8>(MyBtree8&);

template Node16* File::load_node<Node16>(off_t);
template void File::update_node<Node16>(Node16*, off_t, size_t);
template void File::update_node<InternalNode16>(InternalNode16*, off_t, size_t);

template off_t File::insert_primary_index<MyBtree16>(std::string,Record&, MyBtree16&);
template void File::insert_secondary_index<MyBtree16>(std::string,Record&, MyBtree16&,off_t, off_t, int);
template void File::parse_primary_tree<MyBtree16>(MyBtree16&);


template Node32* File::load_node<Node32>(off_t);
template void File::update_node<Node32>(Node32*, off_t, size_t);
template void File::update_node<InternalNode32>(InternalNode32*, off_t, size_t);

template off_t File::insert_primary_index<MyBtree32>(std::string,Record&, MyBtree32&);
template void File::insert_secondary_index<MyBtree32>(std::string,Record&, MyBtree32&,off_t, off_t, int);
template void File::parse_primary_tree<MyBtree32>(MyBtree32&);

template void File::insert_data<MyBtree32, Node32, LeafNode32, InternalNode32>(std::string, Record&, MyBtree32);
template void File::insert_data<MyBtree16, Node16, LeafNode16, InternalNode16>(std::string, Record&, MyBtree16);
template void File::insert_data<MyBtree8, Node8, LeafNode8, InternalNode8>(std::string, Record&, MyBtree8);
template void File::insert_data<MyBtree4, Node4, LeafNode4, InternalNode4>(std::string, Record&, MyBtree4);

template std::vector<Record> File::find<MyBtree32, Node32, InternalNode32,LeafNode32>(std::string, MyBtree32&, off_t);
template std::vector<Record> File::find<MyBtree16, Node16, InternalNode16,LeafNode16>(std::string, MyBtree16&, off_t);
template std::vector<Record> File::find<MyBtree8, Node8, InternalNode8,LeafNode8>(std::string, MyBtree8&, off_t);
template std::vector<Record> File::find<MyBtree4, Node4, InternalNode4, LeafNode4>(std::string, MyBtree4&, off_t);



