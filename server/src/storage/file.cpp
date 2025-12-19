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


template <typename MyBtree>
off_t File::insert_primary_index(std::string key,Record &record, MyBtree &tree, Table &table)
{
    
    using NodeT = typename MyBtree::NodeType;

    NodeT *loaded_node = load_node<NodeT>(table.columns[0].indexLocation); 

    tree.root_node = loaded_node;
    tree.tree_root = table.columns[0].indexLocation;


    //check if this key is in the index already
    if (tree.has_key(key))
    {
        return -1;
    }

    off_t record_location = this->write_record(record);
    tree.insert(key, record_location);




    if(table.columns[0].indexLocation != tree.tree_root)
    {
        table.columns[0].indexLocation = tree.tree_root;
        database->update_index_location(table, 0, table.columns[0].indexLocation);
    }


    return record_location;
}

// template <typename PrimaryTree>
// void File::parse_primary_tree(PrimaryTree &tree)
//{
    
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

// }

template <typename MyBtree>
void File::insert_secondary_index(std::string key, Table &table, MyBtree &tree, off_t record_location, int column_index)
{
    using NodeT = typename MyBtree::NodeType;
    off_t root = table.columns[column_index].indexLocation;
    NodeT *loaded_node = load_node<NodeT>(root); 
    //if(loaded_node->disk_location == 0) tree.init_root(root);

    tree.root_node = loaded_node;
    tree.tree_root = root;
    tree.insert(key,record_location);




    if(root != tree.tree_root)
    {
        table.columns[column_index].indexLocation = tree.tree_root;
        database->update_index_location(table, column_index, table.columns[column_index].indexLocation);
    }
}



template <typename PrimaryBtree, typename MyBtree32, typename MyBtree16, typename MyBtree8, typename MyBtree4>
void File::build_secondary_index(Table& table, int columnIndex,
                                    PrimaryBtree* primaryTree,
                                    MyBtree32* index32,
                                    MyBtree16* index16,
                                    MyBtree8*  index8,
                                    MyBtree4*  index4)
{
    using LeafNodeType = typename PrimaryBtree::LeafNodeType;

    primaryTree->root_node = load_node<typename PrimaryBtree::NodeType>(
        table.columns[0].indexLocation
    );

    LeafNodeType* leaf = primaryTree->find_leftmost_leaf();

    while (leaf != nullptr) {
        for (int i = 0; i < leaf->current_key_count; i++) {
            off_t record_location = leaf->values[i];
            Record record = get_record(record_location, table);

            std::string key = record.get_token(columnIndex, table);
            Type secondaryKeyType = table.columns[columnIndex].type;
            off_t index_location = table.columns[columnIndex].indexLocation;
            std::cout << "Inserting secondary index key: " << key << " at location: " << index_location << std::endl;
            // Dispatch to the correct secondary B+Tree
            switch (secondaryKeyType) {
                case Type::INTEGER:
                    insert_secondary_index(key, table, *index4, record_location, columnIndex);
                    break;
                case Type::CHAR32:
                    insert_secondary_index(key, table, *index32, record_location, columnIndex);
                    break;
                case Type::CHAR16:
                    insert_secondary_index(key, table, *index16, record_location, columnIndex);
                    break;
                case Type::CHAR8:
                    insert_secondary_index(key, table, *index8, record_location, columnIndex);
                    break;
                default:
                    std::cerr << "Unsupported type for secondary index\n";
                    break;
            }
        }


        if (leaf->next_leaf != 0) {
            leaf = static_cast<LeafNodeType*>(load_node<typename PrimaryBtree::NodeType>(leaf->next_leaf));
        } else {
            leaf = nullptr;
        }
    }
}


template<typename MyBtree32, typename MyBtree16, typename MyBtree8, typename MyBtree4>
void File::generate_index(int columnIndex, Table& table,
                          MyBtree32* index32,
                          MyBtree16* index16,
                          MyBtree8*  index8,
                          MyBtree4*  index4)
{
    table.columns[columnIndex].indexLocation = alloc_block();
    update_table_index_location(table, columnIndex, table.columns[columnIndex].indexLocation);

    // Initialize secondary tree root
    switch (table.columns[columnIndex].type) {
        case Type::INTEGER: init_node<typename MyBtree4::NodeType>(table.columns[columnIndex].indexLocation); break;
        case Type::CHAR32:  init_node<typename MyBtree32::NodeType>(table.columns[columnIndex].indexLocation); break;
        case Type::CHAR16:  init_node<typename MyBtree16::NodeType>(table.columns[columnIndex].indexLocation); break;
        case Type::CHAR8:   init_node<typename MyBtree8::NodeType>(table.columns[columnIndex].indexLocation); break;
        default: std::cerr << "Type not supported for indexing\n"; return;
    }

    // Dispatch primary type and build secondary index on demand
    switch (table.columns[0].type) {
        case Type::INTEGER: build_secondary_index<MyBtree4>(table, columnIndex, index4, index32, index16, index8, index4); break;
        case Type::CHAR32:  build_secondary_index<MyBtree32>(table, columnIndex, index32, index32, index16, index8, index4); break;
        case Type::CHAR16:  build_secondary_index<MyBtree16>(table, columnIndex, index16, index32, index16, index8, index4); break;
        case Type::CHAR8:   build_secondary_index<MyBtree8>(table, columnIndex, index8, index32, index16, index8, index4); break;
    }
}





template<typename MyBtree, typename NodeT, typename InternalNodeT, typename LeafNodeT>    
std::vector<Record> File::find(std::string key, MyBtree &index_tree, off_t root_location, Table &table)
{
    std::vector<Record> output;
    index_tree.tree_root = root_location;
    std::vector<off_t> locations = index_tree.search(key);

    for (auto &location: locations)
        output.push_back(get_record(location, table));


    return output;
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

    NodeT* node = reinterpret_cast<NodeT*>(page->buffer);

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
/**
 * @brief Reads a string from buffer until the given delimiter or max length
 */
inline std::string read_until_delim(const std::byte* buffer, int max_len, std::byte delim, int& bytesRead)
{
    bytesRead = 0;
    while (bytesRead < max_len && buffer[bytesRead] != delim)
        bytesRead++;

    std::string result(reinterpret_cast<const char*>(buffer), bytesRead);
    bytesRead++; // include delimiter
    return result;
}

/**
 * @brief Updates the indexLocation of a specific column in a table stored in the table block
 */
off_t File::update_table_index_location(Table &table, int column_index, off_t new_index_value)
{
    Page* page = cache.read_block(table_block_pointer);

    int tableCount = 0;
    std::memcpy(&tableCount, page->buffer, sizeof(int));

    int offset = 12; // table data starts here

    for (int t = 0; t < tableCount; t++)
    {
        int bytesRead = 0;
        std::string tableName = read_until_delim(reinterpret_cast<std::byte*>(page->buffer + offset),
                                                 BLOCK_SIZE - offset, std::byte{0x1F}, bytesRead);
        offset += bytesRead;
        if (tableName == table.name)
        {
            for (size_t col = 0; col < table.columns.size(); col++)
            {
                int bytesRead = 0;
                std::string colName = read_until_delim(reinterpret_cast<std::byte*>(page->buffer + offset),
                                                    BLOCK_SIZE - offset, std::byte{0x1F}, bytesRead);
                offset += bytesRead;

                // Skip type byte + delimiter
                offset += 1 + 1;

                if (col == static_cast<size_t>(column_index))
                {
                    cache.write_to_page(page, offset, &new_index_value, sizeof(off_t), table_block_pointer);
                    return offset;
                }

                // Skip index (8 bytes) + delimiter
                offset += sizeof(off_t) + 1;
            }

        }

        // Skip to end-of-table marker 0x1E
        while (offset < BLOCK_SIZE && static_cast<std::byte>(page->buffer[offset]) != std::byte{0x1E})
            offset++;

        if (offset < BLOCK_SIZE && static_cast<std::byte>(page->buffer[offset]) == std::byte{0x1E})
            offset++;
    }

    return -1;
}



template <typename Node32, typename Node16, typename Node8, typename Node4>
off_t File::insert_table(Table &table)
{
    Page *page = cache.read_block(table_block_pointer);

    int currentTableCount = 0;
    memcpy(&currentTableCount, page->buffer, 4);

    // find write offset (append at end of used table data)
    int writeOffset = 12;
    int tablesFound = 0;

    const std::byte table_end = std::byte{0x1E};

    while (tablesFound < currentTableCount && writeOffset < BLOCK_SIZE)
    {
        if (page->buffer[writeOffset] == table_end)
            tablesFound++;

        writeOffset++;
    }

    // initialize primary index root
    off_t location = alloc_block();
    table.columns[0].indexLocation = location;

    switch (table.columns[0].type)
    {
        case Type::INTEGER:
            init_node<Node4>(location);
            break;
        case Type::CHAR32:
            init_node<Node32>(location);
            break;
        case Type::CHAR16:
            init_node<Node16>(location);
            break;
        case Type::CHAR8:
            init_node<Node8>(location);
            break;
        default:
            std::cerr << "Unsupported type\n";
            return -1;
    }

    // serialize table
    std::vector<std::byte> casted_table = cast_to_bytes(&table);
    
    currentTableCount++;
    cache.write_to_page(page, 0, &currentTableCount, sizeof(currentTableCount), table_block_pointer);
    cache.write_to_page(page, writeOffset, casted_table.data(), casted_table.size(), table_block_pointer);

    return 0;
}




std::vector<Table> File::load_table()
{
    Page* page = cache.read_block(table_block_pointer);
    std::vector<Table> fetched;

    int tableCount = 0;
    memcpy(&tableCount, page->buffer, 4);

    const std::byte table_end = static_cast<std::byte>(0x1E);

    int nextTableStart = 12; // table data starts here

    for (int t = 0; t < tableCount; t++)
    {
        int tableLen = 0;

        // scan until end-of-table marker (0x1E)
        while (nextTableStart + tableLen < BLOCK_SIZE &&
               static_cast<std::byte>(page->buffer[nextTableStart + tableLen]) != table_end)
        {
            tableLen++;
        }

        Table output_table(reinterpret_cast<std::byte*>(page->buffer + nextTableStart), tableLen);
        fetched.push_back(output_table);

        nextTableStart += tableLen + 1; // skip past table_end
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
    node.disk_location = location;
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
Record File::get_record(off_t record_location, Table& table)
{
    off_t page_offset = (record_location/BLOCK_SIZE) * BLOCK_SIZE;
    off_t record_offset = record_location % BLOCK_SIZE;


    Page *page = cache.read_block(page_offset);

    Record record(page->buffer + record_offset, table);

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

        std::cout << "new block :" << free_data_pointer << "\n";

        Page *header_page = cache.read_block(0);

        cache.write_to_page(header_page, HEADER_ROOT_DATA_LOCATION, &free_data_pointer, 8, 0);

    }
    
    
    return record_location;
    
}





Data_Node *File::load_data_node(off_t location)
{
    Page* page = cache.read_block(location);

    // Use page->buffer so we don't alias the Page object and corrupt cache internals
    Data_Node *temp_node = reinterpret_cast<Data_Node*>(page->buffer);

    //todo: edge 
    return temp_node;
}

template off_t File::insert_table<Node32, Node16, Node8, Node4>(Table&);
template void File::generate_index<MyBtree32, MyBtree16, MyBtree8, MyBtree4>(int, Table&,
                          MyBtree32*,
                          MyBtree16*,
                          MyBtree8*,
                          MyBtree4*);

template Node4* File::load_node<Node4>(off_t);
template void File::update_node<Node4>(Node4*, off_t, size_t);
template void File::update_node<InternalNode4>(InternalNode4*, off_t, size_t);

template off_t File::insert_primary_index<MyBtree4>(std::string,Record&, MyBtree4&,Table&);
template void File::insert_secondary_index<MyBtree4>(std::string,Table&, MyBtree4&, off_t, int);
//template void File::parse_primary_tree<MyBtree4>(MyBtree4&);




template Node8* File::load_node<Node8>(off_t);
template void File::update_node<Node8>(Node8*, off_t, size_t);
template void File::update_node<InternalNode8>(InternalNode8*, off_t, size_t);

template off_t File::insert_primary_index<MyBtree8>(std::string,Record&, MyBtree8&,Table&);
template void File::insert_secondary_index<MyBtree8>(std::string,Table&, MyBtree8&, off_t, int);
//template void File::parse_primary_tree<MyBtree8>(MyBtree8&);

template Node16* File::load_node<Node16>(off_t);
template void File::update_node<Node16>(Node16*, off_t, size_t);
template void File::update_node<InternalNode16>(InternalNode16*, off_t, size_t);

template off_t File::insert_primary_index<MyBtree16>(std::string,Record&, MyBtree16&,Table&);
template void File::insert_secondary_index<MyBtree16>(std::string,Table&, MyBtree16&, off_t, int);
//template void File::parse_primary_tree<MyBtree16>(MyBtree16&);


template Node32* File::load_node<Node32>(off_t);
template void File::update_node<Node32>(Node32*, off_t, size_t);
template void File::update_node<InternalNode32>(InternalNode32*, off_t, size_t);

template off_t File::insert_primary_index<MyBtree32>(std::string,Record&, MyBtree32&,Table&);
template void File::insert_secondary_index<MyBtree32>(std::string,Table&, MyBtree32&, off_t, int);
//template void File::parse_primary_tree<MyBtree32>(MyBtree32&);



template std::vector<Record> File::find<MyBtree32, Node32, InternalNode32,LeafNode32>(std::string, MyBtree32&, off_t, Table&);
template std::vector<Record> File::find<MyBtree16, Node16, InternalNode16,LeafNode16>(std::string, MyBtree16&, off_t, Table&);
template std::vector<Record> File::find<MyBtree8, Node8, InternalNode8,LeafNode8>(std::string, MyBtree8&, off_t, Table&);
template std::vector<Record> File::find<MyBtree4, Node4, InternalNode4, LeafNode4>(std::string, MyBtree4&, off_t, Table&);


