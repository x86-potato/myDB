#include "file.hpp"
#include "../core/database.hpp"
#include <algorithm>

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



        //std::cout << "table block,root node, root data: "<< table_block_pointer << " " << free_data_pointer << std::endl;

        //primary_table = load_table();
    }
    else
    {
        header_pointer = 0;

        table_linked_list_head = get_table_block();

        //std::cout << "table block,root node, root data: "<< table_block_pointer   << " " << free_data_pointer << std::endl;

    }
}


template <typename MyBtree>
off_t File::insert_primary_index(
    std::string key,Record &record, MyBtree &tree, Table &table, int txn_id)
{

    using NodeT = typename MyBtree::NodeType;
    Transaction& txn = database->transactions.at(txn_id);
    std::cout << "Transaction " << txn_id << " attempting to insert key: " << key << " into primary index\n";

    //lock the root node pre check
    
    NodeT *loaded_node = load_node<NodeT>(table.columns[0].indexLocation, txn_id);

    //here we would lock it as parent lock 
    off_t root_location = table.columns[0].indexLocation;


    //check if this key is in the index already
    //locks aquired automatically
    if (tree.has_key(key, txn, root_location))
    {
        return -1;
    }

    off_t record_location = this->write_record(record, txn_id, table);
    if(record_location == -1)
    {
        return -1;
    }
    tree.insert(key, record_location,root_location, txn_id);

    std::cout << database->transactions.at(txn_id).txn_id << " inserted record at location " << record_location << " with key " << key << "\n";


    if(table.columns[0].indexLocation != root_location)
    {
        table.columns[0].indexLocation = root_location;
        database->update_index_location(table, 0, table.columns[0].indexLocation);
    }


    return record_location;
}


template <typename MyBtree>
void File::insert_secondary_index(std::string key, Table &table, MyBtree &tree, off_t record_location, int column_index, int txn_id)
{
    using NodeT = typename MyBtree::NodeType;
    off_t root = table.columns[column_index].indexLocation;
    NodeT *loaded_node = load_node<NodeT>(root, txn_id);
    //cache->pin_page(root);
    //tree.root_node = loaded_node;
    //tree.tree_root = root;

    LocationData<typename MyBtree::LeafNodeType> location = tree.locate_exact(key, root, txn_id);

    // Key doesn't exist - insert new entry
    if(location.key_index == -1)
    {
        tree.insert(key, record_location, root, txn_id);
        
        //TODO fix
        //update_node<typename MyBtree::NodeType>(tree.root_node, tree.tree_root, sizeof(typename MyBtree::InternalNodeType));

        // Update root if changed
        if(root != table.columns[column_index].indexLocation)
        {
            table.columns[column_index].indexLocation = root;
            database->update_index_location(table, column_index, table.columns[column_index].indexLocation);
        }
        return;
    }

    // Key exists - add to posting list
    off_t value = location.leaf->values[location.key_index];

    // First duplicate - convert single pointer to posting list
    if(value % 4096 != 0 || value == 0)
    {
        off_t block_location = alloc_block();
        init_posting_block(block_location);

        Posting_Block *block = load_posting_block(block_location);
        block->size = 2;
        block->entries[0] = value;
        block->entries[1] = record_location;

        location.leaf->values[location.key_index] = block_location;
        update_posting_block(block_location, block);
        update_node(location.leaf, location.leaf->disk_location, 4096, 0);
        return;
    }

    // Posting list exists - find block with space
    off_t block_location = value;
    Posting_Block *block = load_posting_block(block_location);

    while(block->size == 509 && block->free_index == -1 && block->next != 0)
    {
        block_location = block->next;
        block = load_posting_block(block_location);
    }

    // Current block has space - insert via freelist or append
    if(block->size < 509 || block->free_index != -1)
    {
        if(block->free_index != -1)
        {
            // Use freelist slot
            int32_t next_free = block->entries[block->free_index];
            block->entries[block->free_index] = record_location;
            block->free_index = next_free;
            block->size++;
        }
        else
        {
            // Append to end
            block->entries[block->size] = record_location;
            block->size++;
        }
        update_posting_block(block_location, block);
        return;
    }

    // All blocks full - allocate new block
    off_t new_block_location = alloc_block();
    init_posting_block(new_block_location);

    Posting_Block *new_block = load_posting_block(new_block_location);
    new_block->prev = block_location;
    new_block->size = 1;
    new_block->entries[0] = record_location;

    block->next = new_block_location;

    update_posting_block(block_location, block);
    update_posting_block(new_block_location, new_block);

    //cache->unpin_page(root);
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

    off_t tree_root = table.columns[0].indexLocation;

    LeafNodeType* leaf = primaryTree->find_leftmost_leaf(tree_root);

    while (leaf != nullptr) {
        for (int i = 0; i < leaf->current_key_count; i++) {
            off_t record_location = leaf->values[i];
            Record record = get_record(record_location, table);

            std::string key = record.get_token(columnIndex, table);
            Type secondaryKeyType = table.columns[columnIndex].type;

            //std::cout << "Inserting secondary index key: " << key << " at location: " << index_location << std::endl;
            // Dispatch to the correct secondary B+Tree
            switch (secondaryKeyType) {
                case Type::INTEGER:
                {

                    uint32_t little_endian_val;
                    little_endian_val = stoi(key);

                    // Convert to big endian
                    uint32_t big_endian_val = htonl(little_endian_val);

                    std::string int_key(reinterpret_cast<const char*>(&big_endian_val), 4);
                    if (int_key.size() != 4) throw std::runtime_error("Key must be 4 bytes");
                    insert_secondary_index(int_key, table, *index4, record_location, columnIndex, 0);
                    break;
                }
                case Type::CHAR32:
                    insert_secondary_index(key, table, *index32, record_location, columnIndex, 0);
                    break;
                case Type::CHAR16:
                    insert_secondary_index(key, table, *index16, record_location, columnIndex, 0);
                    break;
                case Type::CHAR8:
                    insert_secondary_index(key, table, *index8, record_location, columnIndex, 0);
                    break;
                default:
                    std::cerr << "Unsupported type for secondary index\n";
                    break;
            }
        }


        if (leaf->next_leaf != 0) {
            leaf = static_cast<LeafNodeType*>(load_node<typename PrimaryBtree::NodeType>(leaf->next_leaf, 0));
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
        case Type::INTEGER: init_node<typename MyBtree4::NodeType>(table.columns[columnIndex].indexLocation, 0); break;
        case Type::CHAR32:  init_node<typename MyBtree32::NodeType>(table.columns[columnIndex].indexLocation, 0); break;
        case Type::CHAR16:  init_node<typename MyBtree16::NodeType>(table.columns[columnIndex].indexLocation, 0); break;
        case Type::CHAR8:   init_node<typename MyBtree8::NodeType>(table.columns[columnIndex].indexLocation, 0); break;
        default: std::cerr << "Type not supported for indexing\n"; return;
    }

    // Dispatch primary type and build secondary index on demand
    switch (table.columns[0].type) {
        case Type::INTEGER:
            build_secondary_index<MyBtree4>
            (table, columnIndex, index4, index32, index16, index8, index4); break;
        case Type::CHAR32:
            build_secondary_index<MyBtree32>
            (table, columnIndex, index32, index32, index16, index8, index4); break;
        case Type::CHAR16:
            build_secondary_index<MyBtree16>
            (table, columnIndex, index16, index32, index16, index8, index4); break;
        case Type::CHAR8:
            build_secondary_index<MyBtree8>
            (table, columnIndex, index8, index32, index16, index8, index4); break;
        default:
            std::cerr << "Type not supported for indexing\n";
            return;
    }
}




template<typename MyBtree, typename NodeT, typename InternalNodeT, typename LeafNodeT>
std::vector<Record> File::find(std::string key, MyBtree &index_tree, off_t root_location, Table &table)
{
    // std::vector<Record> output;
    // index_tree.tree_root = root_location;
    // std::vector<off_t> locations = index_tree.search(key);

    // for (auto &location: locations)
    //     output.push_back(get_record(location, table));


    // return output;
}


//TODO: needs reword, must allocate many blocks at a time.
off_t File::alloc_block()
{
    std::lock_guard<std::mutex> lock(allocation_mutex);

    //rihgt now dont use, implemnet later
    if(free_linked_list_head != 0 && true == false)
    {
        // Allocate from free list
        off_t allocated_block = free_linked_list_head;
        Page* page = cache.read_block(allocated_block);
        //extract next free block from head of free list

        free_linked_list_head = *reinterpret_cast<off_t*>(page->buffer);
        
        return allocated_block;
    }

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
void File::update_node(NodeT *node, off_t node_location, size_t size, int txn_id)
{
    Page* page = database->transactions.at(txn_id).private_cache_read(node_location);

    database->transactions.at(txn_id).write_to_page(page,0,node, size, node_location);
}
template<typename LeafNodeT>
void File::update_leafnode(LeafNodeT *node, off_t node_location)
{
    Page* page = cache.read_block(node_location);

    cache.write_to_page(page,0,node,BLOCK_SIZE, node_location);

}
void File::update_root_pointer(Table* table,off_t old_location, off_t new_location)
{
    //table = &database->get_table("users");
    //std::cout << "updating root pointer from " << old_location << " to " << new_location << "\n";
    for(size_t i =0; i < table->columns.size(); i++)
    {
        if(table->columns[i].indexLocation == old_location)
        {
            table->columns[i].indexLocation = new_location;
            return;
        }
    }
}
void File::update_root_pointer(off_t old_location, off_t new_location)
{

    for (auto &[t, table] : database->tableMap)
    {
        for(size_t i =0; i < table.columns.size(); i++)
        {
            if(table.columns[i].indexLocation == old_location)
            {
                table.columns[i].indexLocation = new_location;
                return;
            }
        }

        std::cerr << "Warning: Root pointer update failed, old location not found in any table\n";
    }
}
void File::delete_from_posting_list(off_t posting_block_location, off_t record_location)
{
    off_t current_block_location = posting_block_location;

    while (current_block_location != 0)
    {
        Posting_Block* block = load_posting_block(current_block_location);
        bool deleted = false;

        for (int i = 0; i < 509; i++)
        {
            // skip invalid/deleted entries
            if (block->entries[i] < 4096) continue;

            // found the record
            if (block->entries[i] == record_location)
            {
                // add to free list in block
                if (block->free_index == -1)
                {
                    block->free_index = i;
                    block->entries[i] = block->free_index;
                }
                else
                {
                    int32_t old_free_index = block->free_index;
                    block->free_index = i;
                    block->entries[i] = old_free_index;
                }

                block->size--;


                deleted = true;
                break; // only delete one matching entry
            }
        }

        // update block in storage
        update_posting_block(current_block_location, block);

        // check if the block is now empty
        if (block->size == 0)
        {
            if (block->prev != 0)
            {
                Posting_Block* prev = load_posting_block(block->prev);
                prev->next = block->next;
                update_posting_block(block->prev, prev);
            }
            // if prev == 0, caller will handle root pointer update
        }

        // move to next block if not deleted
        if (!deleted && block->next != 0)
        {
            current_block_location = block->next;
        }
        else
        {
            break; // record was deleted or end of list
        }
    }
}


template<typename NodeT>
NodeT* File::load_node(off_t disk_offset, int txn_id) {
    if(database->transactions.find(txn_id) != database->transactions.end())
    {
        Page* page = database->transactions.at(txn_id).private_cache_read(disk_offset);

        if(page != nullptr)
            return reinterpret_cast<NodeT*>(page->buffer);

    }

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
    NodeT *node = load_node<NodeT, LeafNodeT, InternalNodeT>(disk_node_offset, 0);
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
    const std::byte table_end = std::byte{0x1E};

    off_t current_block = table_linked_list_head;

    // walk the linked list of table blocks
    while (current_block != -1) {
        Page* page = cache.read_block(current_block);

        // read next pointer
        off_t next = -1;
        std::memcpy(&next, page->buffer, sizeof(off_t));

        // parse serialized table that follows the pointer
        int offset = sizeof(off_t);
        int bytesRead = 0;
        std::string tableName = read_until_delim(reinterpret_cast<std::byte*>(page->buffer + offset),
                                                 BLOCK_SIZE - offset, std::byte{0x1F}, bytesRead);
        offset += bytesRead; // move past name + delimiter

        if (tableName == table.name) {
            // we found the correct table, now iterate columns to reach desired indexLocation
            for (int col = 0; col < static_cast<int>(table.columns.size()); ++col) {
                // read column name
                int colBytes = 0;
                std::string colName = read_until_delim(reinterpret_cast<std::byte*>(page->buffer + offset),
                                                       BLOCK_SIZE - offset, std::byte{0x1F}, colBytes);
                offset += colBytes;

                // skip type byte and following delimiter
                offset += 1;
                if (offset < BLOCK_SIZE && static_cast<std::byte>(page->buffer[offset]) == std::byte{0x1F})
                    offset++;

                // now offset points at 8-byte indexLocation
                if (col == column_index) {
                    cache.write_to_page(page, offset, &new_index_value, sizeof(off_t), current_block);
                    return offset;
                }

                // skip indexLocation and trailing delimiter (if any)
                offset += sizeof(off_t);
                if (offset < BLOCK_SIZE && static_cast<std::byte>(page->buffer[offset]) == std::byte{0x1F})
                    offset++;
            }

            // if name matched but column index not found, exit loop
            return -1;
        }

        // not the right table, advance to next block
        current_block = next;
    }

    return -1;
}




template <typename Node32, typename Node16, typename Node8, typename Node4>
off_t File::insert_table(Table &table, int txn_id)
{
    Transaction &txn = database->transactions.at(txn_id);

    off_t new_table_location = alloc_block();
    txn.allocated_blocks.push_back(new_table_location);
    
    txn.acquire_exclusive_and_copy_if_needed(new_table_location);
    
    Page* new_table_page = txn.private_cache_read(new_table_location);

    txn.acquire_exclusive_and_copy_if_needed(header_pointer);

    Page* header_page = txn.private_cache_read(header_pointer);

    if(table_linked_list_head == -1)
    {
        table_linked_list_head = new_table_location;
        //also update header so the on-disk head pointer stays current
        txn.copy_page(header_pointer);
        txn.write_to_page(txn.private_cache_read(header_pointer), 0, 
            &table_linked_list_head, sizeof(off_t), header_pointer);

        off_t negative_one = -1;
        txn.write_to_page(new_table_page, 0, &negative_one, 8, new_table_location);
    }
    else
    {
        //if not the first table, set new table as head of linked list and point it to old head
        off_t old_head = table_linked_list_head;
        txn.write_to_page(new_table_page, 0, &old_head, sizeof(off_t), new_table_location);
        table_linked_list_head = new_table_location;
        
        txn.write_to_page(header_page, 0, &table_linked_list_head, sizeof(off_t), header_pointer);
        
    }
    Table_Block table_block;
    table_block.next_table_block = -1;



    // initialize primary index root for the new table
    off_t location = alloc_block();

    txn.acquire_exclusive_and_copy_if_needed(location);
    txn.allocated_blocks.push_back(location);
    //txn.write_to_page(txn.private_cache_read(location), 0, &table_block, sizeof(Table_Block), location);

    table.columns[0].indexLocation = location;

    switch (table.columns[0].type)
    {
        case Type::INTEGER:
            init_node<Node4>(location, txn_id);
            break;
        case Type::CHAR32:
            init_node<Node32>(location, txn_id);
            break;
        case Type::CHAR16:
            init_node<Node16>(location, txn_id);
            break;
        case Type::CHAR8:
            init_node<Node8>(location, txn_id);
            break;
        default:
            std::cerr << "Unsupported type\n";
            return -1;
    }

    // write serialized table data right after the next-pointer

    std::vector<std::byte> casted_table = cast_to_bytes(&table);
    txn.write_to_page(new_table_page, sizeof(off_t), casted_table.data(), casted_table.size(), new_table_location);

    return 0;
}

std::vector<Table> File::load_tables()
{
    std::vector<Table> fetched;

    // traverse linked list of table blocks
    off_t current_block = table_linked_list_head;
    const std::byte table_end = static_cast<std::byte>(0x1E);

    while (current_block != -1) {
        Page* page = cache.read_block(current_block);

        // follow pointer stored in first 8 bytes
        off_t next = -1;
        std::memcpy(&next, page->buffer, sizeof(off_t));

        // locate start of serialized table data immediately after the pointer
        int offset = sizeof(off_t);

        // compute length of the table serialization including end marker
        int tableLen = 0;
        while (offset + tableLen < BLOCK_SIZE &&
               static_cast<std::byte>(page->buffer[offset + tableLen]) != table_end) {
            tableLen++;
        }
        if (offset + tableLen < BLOCK_SIZE &&
            static_cast<std::byte>(page->buffer[offset + tableLen]) == table_end) {
            tableLen++;
        }

        if (tableLen > 0) {
            Table output_table(reinterpret_cast<std::byte*>(page->buffer + offset), tableLen);
            fetched.push_back(output_table);
        }

        current_block = next;
    }

    return fetched;
}

void File::header_block_creation()
{

    Page* header_page = cache.read_block(header_pointer);

    std::byte temp_header[16];

    //set table block pointer
    table_linked_list_head = -1;
    memcpy(&temp_header[0],&table_linked_list_head,8);


    //set root data node pointer
    //free_data_pointer = this->alloc_block();
    //memcpy(&temp_header[8],&free_data_pointer,8);


    //init_data_node(free_data_pointer, 0);

    cache.write_to_page(header_page,0,temp_header,8, 0);


    cache.flush_cache();
}
//void File::init_table_block();
template<typename NodeT>
void File::init_node(off_t location, int txn_id)
{
    NodeT node;
    node.disk_location = location;
    node.is_leaf = true;

    File::update_node(&node, location, sizeof(NodeT), txn_id);

}
void File::init_data_node(off_t location, int txn_id)
{
    Data_Node data;

    data.overflow = false;
    data.next_free_block = 0;

  //  Page* page = database->transactions.at(txn_id).private_cache_read(location); 

 //   memcpy(page->buffer, &data,sizeof(data));

//    database->transactions.at(txn_id).write_to_page(page,0,&data,sizeof(data), location);

}

void File::init_table_block(off_t location)
{
    Page* page = cache.read_block(location);
    //first 4 bytes int
    //next 8 byts overflow

    char temp[8] = {0};
    off_t next_table_block = -1;
    memcpy(temp, &next_table_block, 8);

    cache.write_to_page(page, 0, temp, 8, location);

}

void File::init_posting_block(off_t location)
{
    Page* page = cache.read_block(location);

    Posting_Block block;

    cache.write_to_page(page, 0, &block, sizeof(Posting_Block), location);
}

Posting_Block* File::load_posting_block(off_t location)
{
    Page* page = cache.read_block(location);


    return reinterpret_cast<Posting_Block*>(page->buffer);
}

void File::update_posting_block(off_t location, Posting_Block *block)
{
    Page* page = cache.read_block(location);

    cache.write_to_page(page, 0, block, 4096, location);
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
Record File::get_record(off_t record_location, const Table& table, Transaction* txn)
{

    off_t page_offset = (record_location/BLOCK_SIZE) * BLOCK_SIZE;
    off_t record_offset = record_location % BLOCK_SIZE;

    if(txn != nullptr)
    {
        Page* page = txn->private_cache_read(page_offset);
        
        if(page != nullptr)
            return Record(page->buffer + record_offset, table);
    }

    Page* page = cache.read_block(page_offset);


    Record record(page->buffer + record_offset, table);


    return record;

}

// @brief returns location of the new record
off_t File::write_record(Record &record, int txn_id, Table& table)
{
    //lock the header of the file
    //latch.lock(0);
    //free_data_pointer = cache.read_block(0)->buffer + HEADER_ROOT_DATA_LOCATION;
    off_t data_block_location = table.current_record_block_location;
    Transaction& txn = database->transactions.at(txn_id);
    //latch.unlock(0);

    if(data_block_location == 0)
    {
        //no data block for this table yet, need to allocate one
        data_block_location = alloc_block();
        table.current_record_block_location = data_block_location;


        init_data_node(data_block_location, txn_id);
        Data_Node new_data_node;
        new_data_node.fill_ptr = record.length; 


        txn.acquire_exclusive_and_copy_if_needed(data_block_location);

        //write data node metadata
        txn.write_to_page(txn.private_cache_read(data_block_location), 0, &new_data_node, DATA_NODE_METADATA_SIZE, data_block_location);

        //write record here 
        txn.write_to_page(txn.private_cache_read(data_block_location), DATA_NODE_METADATA_SIZE, record.str.data(), record.length, data_block_location);

        return data_block_location+DATA_NODE_METADATA_SIZE;
    }


    //get read lock on header to find current data block pointer
    //latcher.lock(free_data_pointer);

    //make a copy
    txn.acquire_exclusive_and_copy_if_needed(data_block_location);
    Page* page = txn.private_cache_read(data_block_location);
    Data_Node *node_copy = reinterpret_cast<Data_Node*>(page->buffer);

    

    off_t record_location = 0;
    int space_left = 4072 - node_copy->fill_ptr;



    if(space_left > record.length)
    {

        record_location = data_block_location + (24 + node_copy->fill_ptr);

        memcpy(node_copy->padding + node_copy->fill_ptr, record.str.data(), record.length);
        //cache.write_to_page(page, 24 + node->fill_ptr,record.str.data(), record.length, free_data_pointer);
        node_copy->fill_ptr = node_copy->fill_ptr + record.length;
        database->transactions.at(txn_id).write_to_page(page, 0,node_copy, BLOCK_SIZE, data_block_location);


    }
else
    {
        // =====================================
        // 1. THE CONCURRENCY RE-CHECK
        // =====================================
        // [Lock Shared on Header]
        off_t actual_block_location = 0;
        memcpy(&actual_block_location, cache.read_block(0)->buffer + HEADER_ROOT_DATA_LOCATION, sizeof(off_t));
        // [Unlock Shared on Header]

        if (actual_block_location != data_block_location) {
            // Another thread already fixed the overflow while we were waiting!
            // [Unlock Exclusive on data_block_location]
            
            // Try again from the top with the new pointer
            return write_record(record, txn_id, table); 
        }

        // =====================================
        // 2. THE SAFE ALLOCATION
        // =====================================

        data_block_location = alloc_block();
        // REMOVED: free_data_pointer = data_block_location; (No phantom pointers!)

        Page* header_page = database->transactions.at(txn_id).private_cache_read(0);
        database->transactions.at(txn_id).write_to_page(header_page, HEADER_ROOT_DATA_LOCATION, &data_block_location, sizeof(off_t), 0);

        // =====================================
        // 3. INITIALIZE & WRITE TO NEW PAGE
        // =====================================
        txn.acquire_exclusive_and_copy_if_needed(data_block_location);
        Page *new_page = txn.private_cache_read(data_block_location);

        Data_Node *node = reinterpret_cast<Data_Node*>(new_page->buffer);
        init_data_node(data_block_location, txn_id);

        record_location = data_block_location + (24 + node->fill_ptr);
        memcpy(node->padding + node->fill_ptr, record.str.data(), record.length);
        node->fill_ptr += record.length;

        txn.write_to_page(new_page, 0, node, BLOCK_SIZE, data_block_location);
    }

    return record_location;
}

//@breif updates a record at a given location and column with a new value
int File::update_record(Record &original_record,off_t location, int column_index, std::string& value, Table* table, int txn_id)
{
    //first check if the column had been idnexed
    bool indexed = (table->columns[column_index].indexLocation != -1);



    off_t block_address = (location / BLOCK_SIZE) * BLOCK_SIZE;

    Page* page = cache.read_block(block_address);


    switch (table->columns[column_index].type)
    {
        case Type::INTEGER:
        {
            if(indexed)
            {
                uint32_t old_value = std::stoi(original_record.get_token(column_index, *table));
                uint32_t big_endian_old = htonl(old_value);
                std::string old_key(reinterpret_cast<const char*>(&big_endian_old), 4);

                uint32_t new_value = std::stoi(value);
                uint32_t big_endian_new = htonl(new_value);
                std::string new_key(reinterpret_cast<const char*>(&big_endian_new), 4);

                database->index_tree4.table = table;

                //delete old key
                if (database->index_tree4.delete_key(old_key, location, table->columns[column_index].indexLocation, txn_id) != 0) {
                    return -1;
                }

                //insert new key
                database->index_tree4.insert(new_key, location, table->columns[column_index].indexLocation, txn_id);
            }


            int32_t big_endian_new = std::stoi(value);
            big_endian_new = htonl(big_endian_new);
            original_record.update_column(column_index, value, *table);
            value.append(reinterpret_cast<const char*>(&big_endian_new), sizeof(big_endian_new));


            //write in line, as the record size is not changing
            cache.write_to_page(page, (location%BLOCK_SIZE), original_record.str.data(), original_record.length, block_address);
            break;
        }
        case Type::CHAR32:
        case Type::CHAR16:
        case Type::CHAR8:
            delete_record(original_record, location, *table, txn_id); 
            strip_quotes(value);

            original_record.update_column(column_index, value, *table);

            database->insert(table->name, original_record.to_tokens(*table), txn_id);
            break;

    }







    return location;
}

int File::delete_record(const Record &record, off_t location, const Table& table, int txn_id)
{
    //first we clear out the secondary keys it may have


    int column_index = 0;
    for (auto& index : table.columns)
    {
        if (index.indexLocation != -1)
        {
            off_t block_address = (index.indexLocation/ BLOCK_SIZE) * BLOCK_SIZE;

            std::string key = record.get_token(column_index, table);
            bool is_primary_column = (column_index == 0);
            off_t root_location;

            switch (index.type)
            {
                case Type::INTEGER:
                {
                    int v = stoi(key);
                    v = ntohl(v);
                    std::string key(reinterpret_cast<const char*>(&v), 4);


                    database->index_tree4.table = &const_cast<Table&>(table);
                    root_location = table.columns[column_index].indexLocation;

                    if(is_primary_column)
                    {
                        assert(key.size() == 4);
                        if (database->index_tree4.
                            delete_key(key, location, root_location, txn_id) != 0) {
                            return -1;
                        }
                    }
                    else
                    {
                        off_t posting_list_root = database->index_tree4.locate_exact(key, root_location, txn_id).leaf->values[database->index_tree4.locate_exact(key, root_location, txn_id).key_index];
                        Posting_Block *block = load_posting_block(posting_list_root);
                        delete_from_posting_list(posting_list_root, location);

                        if(block->size == 0)
                        {
                            if(block->next != 0)
                            {
                                database->index_tree4.update_value(key, block->next, root_location, txn_id);
                                break;
                            }
                            else
                            {
                                database->index_tree4.delete_key(key, location, root_location, txn_id);
                                break;
                            }
                        }
                    }
                    break;
                }
                case Type::CHAR32:

                    database->index_tree32.table = &const_cast<Table&>(table);
                    root_location = table.columns[column_index].indexLocation;

                    if(is_primary_column)
                    {
                        assert(key.size() <= 32);
                        if (database->index_tree32.delete_key(key, location, root_location, txn_id) != 0) {
                            return -1;
                        }
                    }
                    else
                    {
                        off_t posting_list_root = database->index_tree32.locate_exact(key, root_location, txn_id).leaf->values[database->index_tree32.locate_exact(key, root_location, txn_id).key_index];
                        if(posting_list_root % 4096 != 0)
                        {
                            database->index_tree32.delete_key(key, location, root_location, txn_id);
                            break;
                        }

                        Posting_Block *block = load_posting_block(posting_list_root);
                        delete_from_posting_list(posting_list_root, location);

                        if(block->size == 0)
                        {
                            if(block->next != 0)
                            {
                                database->index_tree32.update_value(key, block->next, root_location, txn_id);
                                break;
                            }
                            else
                            {
                                database->index_tree32.delete_key(key, location, root_location, txn_id);
                                break;
                            }
                        }
                    }
                    break;

                case Type::CHAR16:
                    root_location = table.columns[column_index].indexLocation;
                    database->index_tree16.table = &const_cast<Table&>(table);

                    if(is_primary_column)
                    {
                        assert(key.size() <= 16);
                        if (database->index_tree16.delete_key(key, location, root_location, txn_id) != 0) {
                            return -1;
                        }
                    }
                    else
                    {
                        off_t posting_list_root = database->index_tree16.locate_exact(key, root_location, txn_id).leaf->values[database->index_tree16.locate_exact(key, root_location, txn_id).key_index];
                        if(posting_list_root % 4096 != 0)
                        {
                            database->index_tree16.delete_key(key, location, root_location, txn_id);
                            break;
                        }

                        Posting_Block *block = load_posting_block(posting_list_root);
                        delete_from_posting_list(posting_list_root, location);

                        if(block->size == 0)
                        {
                            if(block->next != 0)
                            {
                                database->index_tree16.update_value(key, block->next, root_location, txn_id);
                                break;
                            }
                            else
                            {
                                database->index_tree16.delete_key(key, location, root_location, txn_id);
                                break;
                            }
                        }
                    }
                    break;

                case Type::CHAR8:
                    root_location = table.columns[column_index].indexLocation;
                    database->index_tree8.table = &const_cast<Table&>(table);

                    if(is_primary_column)
                    {
                        assert(key.size() <= 8);
                        if (database->index_tree8.delete_key(key, location, root_location, txn_id) != 0) {
                            return -1;
                        }
                    }
                    else
                    {
                        off_t posting_list_root = database->index_tree8.locate_exact(key, root_location, txn_id).leaf->values[database->index_tree8.locate_exact(key, root_location, txn_id).key_index];
                        if(posting_list_root % 4096 != 0)
                        {
                            database->index_tree8.delete_key(key, location, root_location, txn_id);
                            break;
                        }

                        Posting_Block *block = load_posting_block(posting_list_root);
                        delete_from_posting_list(posting_list_root, location);

                        if(block->size == 0)
                        {
                            if(block->next != 0)
                            {
                                database->index_tree8.update_value(key, block->next, root_location, txn_id);
                                break;
                            }
                            else
                            {
                                database->index_tree8.delete_key(key, location, root_location, txn_id);
                                break;
                            }
                        }
                    }
                    break;

                default:
                    throw std::runtime_error("Unsupported column type");
            }
        }

        column_index++;
    }

    return 0;
}

Data_Node *File::load_data_node(off_t location)
{
    Page* page = cache.read_block(location);

    // Use page->buffer so we don't alias the Page object and corrupt cache internals
    Data_Node *temp_node = reinterpret_cast<Data_Node*>(page->buffer);

    //todo: edge
    return temp_node;
}



template off_t File::insert_table<Node32, Node16, Node8, Node4>(Table&, int);
template void File::generate_index<MyBtree32, MyBtree16, MyBtree8, MyBtree4>(int, Table&,
                          MyBtree32*,
                          MyBtree16*,
                          MyBtree8*,
                          MyBtree4*);

template Node4* File::load_node<Node4>(off_t, int);
template void File::update_node<Node4>(Node4*, off_t, size_t, int);
template void File::update_node<InternalNode4>(InternalNode4*, off_t, size_t, int);

template off_t File::insert_primary_index<MyBtree4>(std::string,Record&, MyBtree4&,Table&, int);
template void File::insert_secondary_index<MyBtree4>(std::string,Table&, MyBtree4&, off_t, int, int);
//template void File::parse_primary_tree<MyBtree4>(MyBtree4&);




template Node8* File::load_node<Node8>(off_t, int);
template void File::update_node<Node8>(Node8*, off_t, size_t, int);
template void File::update_node<InternalNode8>(InternalNode8*, off_t, size_t, int);

template off_t File::insert_primary_index<MyBtree8>(std::string,Record&, MyBtree8&,Table&, int);
template void File::insert_secondary_index<MyBtree8>(std::string,Table&, MyBtree8&, off_t, int, int);
//template void File::parse_primary_tree<MyBtree8>(MyBtree8&);

template Node16* File::load_node<Node16>(off_t, int);
template void File::update_node<Node16>(Node16*, off_t, size_t, int);
template void File::update_node<InternalNode16>(InternalNode16*, off_t, size_t, int);



template off_t File::insert_primary_index<MyBtree16>(std::string,Record&, MyBtree16&,Table&,int);
template void File::insert_secondary_index<MyBtree16>(std::string,Table&, MyBtree16&, off_t, int, int);
//template void File::parse_primary_tree<MyBtree16>(MyBtree16&);


template Node32* File::load_node<Node32>(off_t, int);
template void File::update_node<Node32>(Node32*, off_t, size_t, int);
template void File::update_node<InternalNode32>(InternalNode32*, off_t, size_t, int);

template off_t File::insert_primary_index<MyBtree32>(std::string,Record&, MyBtree32&,Table&, int);
template void File::insert_secondary_index<MyBtree32>(std::string,Table&, MyBtree32&, off_t, int, int);
//template void File::parse_primary_tree<MyBtree32>(MyBtree32&);



template std::vector<Record> File::find<MyBtree32, Node32, InternalNode32,LeafNode32>(std::string, MyBtree32&, off_t, Table&);
template std::vector<Record> File::find<MyBtree16, Node16, InternalNode16,LeafNode16>(std::string, MyBtree16&, off_t, Table&);
template std::vector<Record> File::find<MyBtree8, Node8, InternalNode8,LeafNode8>(std::string, MyBtree8&, off_t, Table&);
template std::vector<Record> File::find<MyBtree4, Node4, InternalNode4, LeafNode4>(std::string, MyBtree4&, off_t, Table&);
