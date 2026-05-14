#include "btree.hpp"
#include <cstring>

class File;


template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
bool BtreePlus<NodeT, LeafNodeT, InternalNodeT>::is_less_than(const char left[KeyLen], const char right[KeyLen])
{
    return std::memcmp(left, right, KeyLen) < 0;
}

template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
BtreePlus<NodeT, LeafNodeT, InternalNodeT>::BtreePlus(File *file) : file(file)
{

}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
BtreePlus<NodeT, LeafNodeT, InternalNodeT>::BtreePlus()
{

}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>
::insert(std::string insert_string, off_t &record_location, Column& column, Transaction& txn,
std::vector<off_t>& path_stack)
{
    char buffer[KeyLen] = {0};
    std::memcpy(buffer, insert_string.c_str(), insert_string.length());

    // 1. Get the target leaf from the stack that has_key generated
    if (path_stack.empty()) {
        throw std::runtime_error("Path stack is empty! has_key must be called first.");
    }
    
    off_t leaf_location = path_stack.back();
    
    // 2. CRITICAL FIX: Pop the leaf off the stack so the stack now ONLY contains ancestors!
    path_stack.pop_back(); 

    // 3. Load the leaf directly
    NodeT* cursor = file->load_node<NodeT>(leaf_location, txn);

    // 4. Insert the new row into the leaf
    Insert_Up_Data data = {};
    memcpy(data.key, buffer, KeyLen);
    data.left_child = 0;
    data.right_child = record_location; // The physical row location
    
    insert_key_into_node(data, cursor);

    // 5. Update the leaf on disk 
    // CRITICAL FIX: Use sizeof(LeafNodeT), not InternalNodeT!
    file->update_node(cursor, cursor->disk_location, sizeof(LeafNodeT), txn);

    // 6. Split if full
    if(cursor->current_key_count == MaxKeys)
    {
        // Because we popped the leaf on line 16, path_stack is now perfectly set up for the parent!
        split_leaf(cursor, txn, column, path_stack);
    }
}
//@assume tree_root is already set

// template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
// int BtreePlus<NodeT, LeafNodeT, InternalNodeT>
// ::delete_key(std::string delete_string, off_t record_location, Column& column, Transaction& txn)
// {
//     // char buffer[KeyLen] = {0};
//     // std::memcpy(buffer, delete_string.c_str(), delete_string.length());

//     // SharedPageGuard current_guard = SharedPageGuard(column.indexLocation, txn);


//     // NodeT* cursor = static_cast<NodeT*>(traverse_to_leaf(buffer, current_guard, column.indexLocation, txn));
//     // int index = get_first_key_index_gte(buffer, static_cast<LeafNodeT*>(cursor));

//     // if(memcmp(cursor->keys[index], buffer, KeyLen) != 0)
//     // {
//     //     InternalNodeT* parent;
//     //     parent = static_cast<InternalNodeT*>(file->load_node<NodeT>(cursor->parent, txn));
//     //     InternalNodeT* parents_parent = nullptr;
//     //     if(parent != nullptr && parent->parent != 0)
//     //         parents_parent = static_cast<InternalNodeT*>(file->load_node<NodeT>(parent->parent, txn));


//     //     return -1;
//     // }


//     // off_t cursor_location = cursor->disk_location;

//     // LeafNodeT* leaf_cursor = static_cast<LeafNodeT*>(cursor);
//     // if(leaf_cursor->values[index] % 4096 == 0 && leaf_cursor->values[index] != record_location)
//     // {
//     //     std::cout << "posting list emptied for key " << delete_string << "\n";
//     //     return -1;
//     // }

//     // delete_index_in_node(index, cursor, false, txn);
//     // file->update_node(cursor, cursor->disk_location, sizeof(LeafNodeT), txn);


//     // if (cursor->parent == 0 && cursor->current_key_count == 0)
//     // {
//     //     // Tree is now empty
//     //     //tree_root = 0;
//     //     //root_node = nullptr;
//     //     //std::cout << "root pointer set from " << cursor->disk_location << " to -1\n";
//     //     //file->update_root_pointer(table, cursor->disk_location, -1);
//     //     return 0;
//     // }

//     // // Only handle underflow if this isn't the root
//     // if(check_underflow(cursor, txn) && cursor->parent != 0)
//     // {
//     //     NodeT *parent = file->load_node<NodeT>(cursor->parent, txn);
//     //     InternalNodeT* parent_cast = static_cast<InternalNodeT*>(parent);

//     //     if (parent_cast->current_key_count == 0 && parent_cast->parent == 0)
//     //     {
//     //         // The parent is a root with 0 keys. It is superfluous.
//     //         // Current node (cursor) should be the new root.

//     //         cursor->parent = 0;
//     //         //off_t old_root = tree_root;
//     //         off_t new_root = cursor->disk_location;

//     //         //TODO: fix file->update_root_pointer(table, old_root, new_root);
//     //         //std::cout << "[FIX] Collapsing 0-key root. New root: " << new_root << "\n";

//     //         //tree_root = new_root;
//     //         //root_node = cursor; // Update cache

//     //         file->update_node(cursor, cursor->disk_location, sizeof(LeafNodeT), txn);

//     //         // We are now the root, so we don't need to balance with siblings
//     //         return 0;
//     //     }


//     //     int child_index = find_child_index(parent_cast, cursor->disk_location);

//     //     bool borrow_success = attempt_borrow(cursor, parent_cast, child_index, txn);

//     //     if(!borrow_success)
//     //     {
//     //         off_t surviving_node_location;
//     //         if(child_index != 0) {
//     //             surviving_node_location = parent_cast->children[child_index - 1];
//     //         } else {
//     //             surviving_node_location = cursor->disk_location;
//     //         }

//     //         leaf_merge(parent_cast, cursor, child_index, txn);

//     //         // Reload root in case it changed during merge
//     //         //root_node = file->load_node<NodeT>(tree_root);


//     //         return 0;
//     //         cursor = file->load_node<NodeT>(surviving_node_location, txn);
//     //     }
//     //     else
//     //     {
//     //         cursor = file->load_node<NodeT>(cursor_location, txn);
//     //     }
//     // }

//     // // Reload root in case structure changed
//     // //root_node = file->load_node<NodeT>(tree_root);

//     // // Only update parent separator if we still have a parent
//     // if(cursor->current_key_count > 0 && cursor->parent != 0)
//     // {
//     //     NodeT *parent = file->load_node<NodeT>(cursor->parent, txn);
//     //     InternalNodeT* parent_cast = static_cast<InternalNodeT*>(parent);
//     //     int child_index = find_child_index(parent_cast, cursor->disk_location);

//     //     if(child_index != 0 && child_index != -1)
//     //     {
//     //         memcpy(parent_cast->keys[child_index-1], cursor->keys[0], KeyLen);
//     //         file->update_node(parent_cast, parent_cast->disk_location, sizeof(InternalNodeT), txn);
//     //     }
//     // }

//     return 0;
// }

template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::update_value(std::string key, off_t new_value, off_t root_location, Transaction& txn)
{
    char buffer[KeyLen] = {0};
    std::memcpy(buffer, key.c_str(), key.length());

    SharedPageGuard current_guard(root_location, txn);

    LeafNodeT* leaf = traverse_to_leaf(buffer, current_guard, root_location, txn);
    int index = get_first_key_index_gte(buffer, static_cast<LeafNodeT*>(leaf));

    if(memcmp(leaf->keys[index], buffer, KeyLen) != 0)
    {
        return;
    }

    leaf->values[index] = new_value;
    file->update_node(static_cast<NodeT*>(leaf), leaf->disk_location, sizeof(LeafNodeT), txn);
    return;
}

template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
off_t BtreePlus<NodeT, LeafNodeT, InternalNodeT>::get_next_leftmost_node_pointer(char* to_search,InternalNodeT *node)
{
    for (int i = 0; i < node->current_key_count; i++)
    {
        if (memcmp(to_search, node->keys[i], KeyLen) < 0)
        {
            return node->children[i];
        }
    }
    return node->children[node->current_key_count];
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
LocationData<LeafNodeT> BtreePlus<NodeT, LeafNodeT, InternalNodeT>::
locate_exact(std::string key, const Column& column, Transaction& txn)
{
    LocationData<LeafNodeT> output;

    SharedPageGuard current_guard(column.indexLocation, txn);

    char buffer[KeyLen] = {0};
    std::memcpy(buffer, key.c_str(), key.length());

    NodeT* cursor = file->load_node<NodeT>(column.indexLocation, txn);
    if (cursor->current_key_count == 0)
    {
        output.key_index = -1;
        output.leaf = nullptr;
        return output;
    }

    LeafNodeT* leaf_cursor = traverse_to_leaf(buffer, current_guard, column.indexLocation, txn);

    output.key_index =get_first_key_index_gte(buffer, leaf_cursor);

    if (output.key_index == -1) return output;
    if (memcmp(leaf_cursor->keys[output.key_index], buffer, KeyLen) != 0)
    {
        output.key_index = -1;

    }

    output.leaf = leaf_cursor;

    return output;
}

template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
LocationData<LeafNodeT> BtreePlus<NodeT, LeafNodeT, InternalNodeT>::
locate_gt(std::string key, const Column& column, Transaction& txn)
{
    LocationData<LeafNodeT> output;

    char buffer[KeyLen] = {0};
    std::memcpy(buffer, key.c_str(), key.length());


    SharedPageGuard current_guard(column.indexLocation, txn);
    NodeT* cursor = file->load_node<NodeT>(column.indexLocation, txn);

    if (cursor->current_key_count == 0)
    {
        output.leaf = nullptr;
        return output;
    }

    LeafNodeT* leaf = traverse_to_leaf(buffer, current_guard, column.indexLocation, txn);
    output.key_index = get_first_key_index_gte(buffer, static_cast<LeafNodeT*>(leaf));

    if(output.key_index == -1) return output;

    if (std::memcmp(leaf->keys[output.key_index], buffer, KeyLen) == 0)
    {
        output.key_index += 1;
        if (output.key_index >= leaf->current_key_count)
        {
            if (leaf->next_leaf != 0)
            {
                current_guard = SharedPageGuard(leaf->next_leaf, txn);
                leaf = static_cast<LeafNodeT*>(file->load_node<NodeT>(leaf->next_leaf, txn));
                output.key_index = 0;
            }
            else
            {
                output.key_index = -1;
                output.leaf = nullptr;
                return output;
            }
        }
    }

    output.leaf_guard = std::move(current_guard);
    output.leaf = leaf;
    return output;
}

template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
LocationData<LeafNodeT> BtreePlus<NodeT, LeafNodeT, InternalNodeT>::
locate_gte(std::string key, const Column& column, Transaction& txn)
{
    LocationData<LeafNodeT> output;

    char buffer[KeyLen] = {0};
    std::memcpy(buffer, key.c_str(), key.length());

    // 1. Lock the root
    SharedPageGuard current_guard(column.indexLocation, txn);
    NodeT* cursor = file->load_node<NodeT>(column.indexLocation, txn);

    if (cursor->current_key_count == 0)
    {
        output.key_index = -1;
        output.leaf = nullptr;
        // current_guard goes out of scope and safely drops the lock
        return output; 
    }

    // 2. Traverse securely down to the leaf
    // FIX: Pass current_guard by reference and use column.indexLocation
    LeafNodeT* leaf = traverse_to_leaf(buffer, current_guard, column.indexLocation, txn);

    output.key_index = get_first_key_index_gte(buffer, leaf);

    // 3. Boundary Check & Lateral Crabbing
    if (output.key_index >= leaf->current_key_count) {
        if (leaf->next_leaf != 0) {
            // FIX: LATERAL CRABBING - Safely transition the lock to the sibling
            current_guard = SharedPageGuard(leaf->next_leaf, txn);
            
            leaf = static_cast<LeafNodeT*>(file->load_node<NodeT>(leaf->next_leaf, txn));
            output.key_index = 0;
        }
        else
        {
            // No more keys in tree (key is greater than absolute max element)
            output.key_index = -1;
            output.leaf = nullptr;
            return output; // current_guard goes out of scope and unlocks
        }
    }

    // 4. Output Assignment & Lock Transfer
    output.leaf = leaf;
    
    // FIX: Transfer ownership of the lock to the caller
    output.leaf_guard = std::move(current_guard);

    return output;
}


template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
LocationData<LeafNodeT> BtreePlus<NodeT, LeafNodeT, InternalNodeT>
::locate_start(const Column& column, Transaction &txn)
{
    LocationData<LeafNodeT> output;

    SharedPageGuard current_guard(column.indexLocation, txn);

    NodeT* cursor = file->load_node<NodeT>(column.indexLocation, txn);


    if(cursor->current_key_count == 0)
    {
        output.key_index = -1;
        output.leaf = nullptr;

        return output;
    }

    while(!cursor->is_leaf)
    {
        InternalNodeT *cursor_cast = static_cast<InternalNodeT*>(cursor);

        current_guard = SharedPageGuard(cursor_cast->children[0], txn);

        cursor = file->load_node<NodeT>(cursor_cast->children[0], txn);
    }


    output.leaf_guard = std::move(current_guard);
    output.key_index = 0;
    output.leaf = static_cast<LeafNodeT*>(cursor);
    

    return output;
}

template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
std::vector<off_t> BtreePlus<NodeT, LeafNodeT, InternalNodeT>
::search(std::string search_string, off_t root_location, Transaction& txn)
{
    std::vector<off_t> output;
    char search_key[KeyLen] = {0};
    std::memcpy(search_key, search_string.c_str(), search_string.length()); // keep strncpy

    NodeT* cursor = file->load_node<NodeT>(root_location, txn);

    while (!cursor->is_leaf)
    {
        InternalNodeT* cursor_cast = static_cast<InternalNodeT*>(cursor);
        cursor = file->load_node<NodeT>(search_recursive(search_key, cursor_cast), txn);
    }
    LeafNodeT* leaf = static_cast<LeafNodeT*>(cursor); // traverse tree down to first leaf

    while (leaf)
    {
        //std::cout << "reading on: " << leaf->disk_location;
        for (int i = 0; i < leaf->current_key_count; i++)
        {
            if (std::memcmp(leaf->keys[i], search_key, KeyLen) == 0)
            {
                output.push_back(leaf->values[i]);
            }


        }
        if(leaf->next_leaf != 0)
        {
            leaf = static_cast<LeafNodeT*>(file->load_node<NodeT>(leaf->next_leaf, txn));
            if (std::memcmp(leaf->keys[0], search_key, KeyLen) != 0)
                leaf=nullptr;

        }
        else
        {
            leaf = nullptr;
        }

    }

    //print_tree();

    return output; // key not found
}

template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
bool BtreePlus<NodeT, LeafNodeT, InternalNodeT>
::has_key(const std::string &key, Transaction& txn, Column &column, std::vector<off_t>& path_stack)
{
    char buffer[KeyLen] = {0};
    std::memcpy(buffer, key.c_str(), key.length());

    // Ensure we start with a clean slate if the vector is being reused
    path_stack.clear(); 

    off_t current_root_location = column.indexLocation;

    // --- PIN THE ROOT ---
    while (true)
    {
        current_root_location = column.indexLocation; 
        file->load_node<NodeT>(current_root_location, txn); 
        txn.try_temp_lock(current_root_location); 

        if (current_root_location == column.indexLocation) {
            break;
        }
        txn.try_release_temp_lock(current_root_location); 
    }

    NodeT *cursor = file->load_node<NodeT>(current_root_location, txn);

    // --- 1. ACQUIRE OWNERSHIP OF ROOT ---
    off_t active_root_loc = cursor->disk_location;
    txn.acquire_ownership_and_copy_if_needed(active_root_loc);

    if (active_root_loc != cursor->disk_location) 
    {
        // Root was copied to a new location!
        cursor->disk_location = active_root_loc;
        current_root_location = active_root_loc;
        column.indexLocation = active_root_loc; 
        
        // Note: Update your DB catalog here to persist new root loc!
    }

    // Lock the confirmed, active root and add to our path
    txn.try_temp_lock(current_root_location);
    path_stack.push_back(current_root_location);

    // --- DOWNWARD TRAVERSAL ---
    while(!cursor->is_leaf)
    {
        InternalNodeT* internal = static_cast<InternalNodeT*>(cursor);
        
        // Find the specific child we are descending into
        off_t next_node_location = get_next_leftmost_node_pointer(buffer, internal);
        NodeT* child = file->load_node<NodeT>(next_node_location, txn);

        // --- 2. ACQUIRE OWNERSHIP OF SPECIFIC CHILD ---
        off_t old_child_loc = child->disk_location;
        off_t active_child_loc = old_child_loc;
        
        txn.acquire_ownership_and_copy_if_needed(active_child_loc);

        if (active_child_loc != old_child_loc) 
        {
            // The child was copied! We MUST update the parent's pointer immediately.
            child->disk_location = active_child_loc;
            
            for(int i = 0; i <= internal->current_key_count; i++) {
                if (internal->children[i] == old_child_loc) {
                    internal->children[i] = active_child_loc;
                    break;
                }
            }
            
            // Persist the parent since we modified its child array
            // (This is perfectly safe because we currently hold the parent's exclusive lock)
            file->update_node(internal, internal->disk_location, sizeof(InternalNodeT), txn);
            
            next_node_location = active_child_loc;
        }

        // Lock the child and push to our path stack
        txn.try_temp_lock(next_node_location);
        path_stack.push_back(next_node_location);

        // --- 3. SAFETY CHECK & CRABBING ---
        bool child_is_safe = (child->current_key_count < MaxKeys - 1);

        if(child_is_safe)
        {
            // If the child is safe, no split will propagate up past it.
            // Release ALL ancestor locks.
            for (size_t i = 0; i < path_stack.size() - 1; ++i) {
                txn.try_release_temp_lock(path_stack[i]);
            }
            
            // Wipe the stack, keeping ONLY the safe child as the new top of the chain.
            // Since it's safe, we will never need to split anything above it.
            path_stack.erase(path_stack.begin(), path_stack.end() - 1);
        }

        cursor = child;
    }

    // --- LEAF REACHED ---
    bool found = (leaf_contains(cursor, key) != -1);

    if(found) {
        txn.release_temp_locks(); // Assuming mutator aborts if key already exists
    } else {
        // We found the path. Promote locks.
        // `path_stack` now contains exactly the stack of nodes needed for a bottom-up split.
        txn.promote_temp_locks_to_permanent();
    }

    return found;
}

//@called when we want to lock location of the key for secondary insert.
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
bool BtreePlus<NodeT, LeafNodeT, InternalNodeT>
::lock_secondary_key(const std::string &key, Transaction& txn, off_t root_location)
{
    char buffer[KeyLen] = {0};
    std::memcpy(buffer, key.c_str(), key.length());


    NodeT *cursor = file->load_node<NodeT>(root_location, txn);


    while(!cursor->is_leaf)
    {
        InternalNodeT* internal = static_cast<InternalNodeT*>(cursor);
        off_t next_node_location = get_next_leftmost_node_pointer(buffer, internal);

        NodeT* child = file->load_node<NodeT>(next_node_location, txn);

        // Lock the child and add it to our tracker BEFORE evaluating safety

        cursor = child;
    }

    bool found = (leaf_contains(cursor, key) != -1);

    // Assuming txn internally tracks what is still locked via try_release_temp_lock:
    // If found, we drop everything. If not, we keep the safe path locked for insertion.
    if(found)
    {
        txn.acquire_ownership_and_copy_if_needed(cursor->disk_location);
    }
    else
    {
        std::cout << "should not hit";
    }

    return found;
}

template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::print_tree()
{
    //root_node = file->load_node<NodeT>(tree_root);
    //std::cout << "B+ Tree structure (root at " << root_node->disk_location << "):\n";
    //print_recursive(root_node, 0, std::cout);
}


template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::init_root(off_t location, Transaction& txn)
{
    NodeT *firstNode = file->load_node<NodeT>(location, txn);
    firstNode->is_leaf = true;
    firstNode->disk_location = location;

    file->update_node<NodeT>(firstNode, location, sizeof(InternalNodeT), txn);
    //root_node = file->load_node<NodeT>(location);
    //tree_root = location;

}
// template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
// void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::leaf_merge(
//     InternalNodeT* parent,
//     NodeT* current,
//     int child_index,
//     Transaction& txn)
// {
// }
    // //std::cout << "leaf merge called"<<"\n";
    // if (child_index != 0)  // merge current into left
    // {
    //     NodeT* left_node = file->load_node<NodeT>(parent->children[child_index - 1], txn);
    //     LeafNodeT* left  = static_cast<LeafNodeT*>(left_node);
    //     LeafNodeT* curr  = static_cast<LeafNodeT*>(current);

    //     left->next_leaf = curr->next_leaf;

    //     int keys_to_move = current->current_key_count;
    //     for (int i = 0; i < keys_to_move; i++)
    //     {
    //         std::memcpy(left->keys[left->current_key_count], current->keys[i], KeyLen);
    //         left->values[left->current_key_count] = curr->values[i];
    //         left->current_key_count++;
    //     }

    //     // Explicitly zero out the entire key and value arrays of the emptied node
    //     for (int i = 0; i < MaxKeys; ++i)
    //     {
    //         std::fill(std::begin(current->keys[i]), std::end(current->keys[i]), 0);
    //         curr->values[i] = 0;
    //     }
    //     current->current_key_count = 0;

    //     file->update_node(left_node, left_node->disk_location, sizeof(LeafNodeT), txn);
    //     file->update_node(current, current->disk_location, sizeof(LeafNodeT), txn);

    //     int separator_index = child_index - 1;
    //     delete_index_in_node(separator_index, parent, child_index, txn);  // remove separator + pointer to current

    //     file->update_node(parent, parent->disk_location, sizeof(InternalNodeT), txn);

    //     if (check_underflow(parent, txn))
    //     {
    //         internal_underflow(parent, txn);
    //     }

    //     //TODO: check if the parent disk location is root not parent 0 may fail
    //     if (parent->parent == 0&& parent->current_key_count == 0)
    //     {
    //         NodeT* left_node = file->load_node<NodeT>(parent->children[child_index - 1], txn);
    //         left_node->parent = 0;

    //         //off_t old_root = tree_root;
    //         off_t new_root = left_node->disk_location;

    //         //TODO: FIX:file->update_root_pointer(table, old_root, new_root);
    //         //std::cout << "[ROOT PROMOTION via LEAF MERGE] Empty root -> new root: " << new_root << "\n";
    //         //tree_root = new_root;
    //         //root_node = left_node;

    //         file->update_node(left_node, left_node->disk_location, sizeof(LeafNodeT), txn);

    //         return;
    //     }
    // }
    // else  // merge right into current
    // {
    //     NodeT* right_node = file->load_node<NodeT>(parent->children[child_index + 1], txn);
    //     LeafNodeT* right = static_cast<LeafNodeT*>(right_node);
    //     LeafNodeT* curr  = static_cast<LeafNodeT*>(current);

    //     curr->next_leaf = right->next_leaf;

    //     int keys_to_move = right->current_key_count;
    //     for (int i = 0; i < keys_to_move; i++)
    //     {
    //         std::memcpy(current->keys[current->current_key_count], right->keys[i], KeyLen);
    //         curr->values[current->current_key_count] = right->values[i];
    //         current->current_key_count++;
    //     }

    //     // Explicitly zero out the entire key and value arrays of the emptied right node
    //     for (int i = 0; i < MaxKeys; ++i)
    //     {
    //         std::fill(std::begin(right->keys[i]), std::end(right->keys[i]), 0);
    //         right->values[i] = 0;
    //     }
    //     right->current_key_count = 0;

    //     file->update_node(current, current->disk_location, sizeof(LeafNodeT), txn);
    //     file->update_node(right_node, right_node->disk_location, sizeof(LeafNodeT), txn);

    //     int separator_index = child_index;
    //     delete_index_in_node(separator_index, parent, child_index + 1, txn);  // remove separator + pointer to right

    //     file->update_node(parent, parent->disk_location, sizeof(InternalNodeT), txn);

    //     if (check_underflow(parent, txn))
    //     {
    //         internal_underflow(parent, txn);
    //     }


    //     if (parent->parent == 0 && parent->current_key_count == 0)
    //     {
    //         current->parent = 0;

    //         //off_t old_root = tree_root;
    //         off_t new_root = current->disk_location;

    //         //TODO:fix file->update_root_pointer(table, old_root, new_root);
    //         //std::cout << "[ROOT PROMOTION via LEAF MERGE] Empty root -> new root: " << new_root << "\n";
    //         //tree_root = new_root;
    //         //root_node = current;

    //         file->update_node(current, current->disk_location, sizeof(LeafNodeT), txn);

    //         return;
    //     }
    // }
// }
// template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
// void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::internal_underflow(NodeT* node, Transaction& txn)
// {
    // if (!node) return;

    // InternalNodeT* node_cast = static_cast<InternalNodeT*>(node);

    // // Early exit: if this node no longer underflows, stop recursion
    // if (!check_underflow(node, txn))
    //     return;

    // // Root case: if this is already the root and empty (0 keys, internal node)
    // if (node_cast->parent == 0)
    // {
    //     if (node_cast->current_key_count == 0 && !node_cast->is_leaf)
    //     {
    //         off_t new_root_loc = node_cast->children[0];
    //         if (new_root_loc == 0)
    //         {
    //             std::cerr << "ERROR: Empty root with no child\n";
    //             return;
    //         }

    //         NodeT* new_root_node = file->load_node<NodeT>(new_root_loc, txn);
    //         new_root_node->parent = 0;

    //         off_t old_root = node_cast->disk_location;

    //         file->update_root_pointer(table, old_root, new_root_loc);
    //         //tree_root   = new_root_loc;
    //         //root_node   = new_root_node;

    //         //std::cout << "[ROOT PROMOTION] Empty root → new root: " << tree_root << "\n";

    //         file->update_node(new_root_node, new_root_loc, sizeof(InternalNodeT), txn);

    //         // Optional: mark old root as free / zero it out if you have free list
    //     }
    //     return;  // root cannot merge upward — always stop here
    // }

    // // Non-root case
    // InternalNodeT* parent = static_cast<InternalNodeT*>(file->load_node<NodeT>(node_cast->parent, txn));
    // if (!parent)
    // {
    //     std::cerr << "ERROR: Node claims parent but load failed\n";
    //     return;
    // }

    // int child_index = find_child_index(parent, node_cast->disk_location);
    // if (child_index < 0)
    // {
    //     std::cerr << "ERROR: Cannot find self in parent's children\n";
    //     return;
    // }

    // // Try to borrow first
    // bool borrowed = attempt_borrow_internal(node_cast, parent, child_index, txn);

    // if (!borrowed)
    // {
    //     // Merge — this may cause parent to underflow
    //     merge_internal(node_cast, parent, child_index, txn);

    //     // After merge, re-check if parent now underflows
    //     // (merge_internal should have already handled root promotion if parent was root)
    //     if (check_underflow(static_cast<NodeT*>(parent), txn) && parent->parent != 0)
    //     {
    //         // Only recurse if parent is still underflown **and** not root
    //         internal_underflow(static_cast<NodeT*>(parent), txn);
    //     }
    // }

    // // Important: do NOT put root promotion logic here anymore
    // // It belongs either in merge_internal (when parent is root) or in the root-special case above
// }
// template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
// void BtreePlus<NodeT, LeafNodeT, InternalNodeT>
// ::merge_internal(InternalNodeT* node, InternalNodeT* parent, int child_index, Transaction& txn)
// {
    // //std::cout << "merge internal called"<<"\n";
    // //first we insert parent key into the back of this one
    // if(child_index != 0) //merge with left
    // {
    //     //std::cout << "merge left";
    //     int separator_index = child_index-1;
    //     InternalNodeT *left = static_cast<InternalNodeT*>(file->load_node<NodeT>(parent->children[child_index-1], txn));

    //     memcpy(left->keys[left->current_key_count], parent->keys[separator_index],KeyLen);
    //     left->current_key_count++;
    //     //move nodes children into left
    //     int left_key_count = left->current_key_count;
    //     int left_child_start = left->current_key_count; // keys already incremented, this is correct slot
    //     for (int i = 0; i < node->current_key_count + 1; i++) {
    //         left->children[left_child_start + i] = node->children[i];
    //     }

    //     // Move node's keys into left
    //     int current_key_iterator = 0;
    //     while (node->current_key_count > 0) {
    //         memcpy(left->keys[left->current_key_count], node->keys[current_key_iterator], KeyLen);
    //         left->current_key_count++;
    //         current_key_iterator++;
    //         node->current_key_count--;
    //     }

    //     // FIX: NOW re-parent ALL of left's children (including newly moved ones) and persist
    //     for (int i = 0; i < left->current_key_count + 1; i++) {
    //         if (left->children[i]) {
    //             NodeT* child = file->load_node<NodeT>(left->children[i], txn);
    //             child->parent = left->disk_location;
    //             file->update_node(child, child->disk_location, sizeof(InternalNodeT), txn);
    //         }
    //     }
    //     delete_index_in_node(separator_index, parent, child_index, txn);
    //     parent->children[child_index-1] = left->disk_location;

    //     file->update_node(parent, parent->disk_location, sizeof(InternalNodeT), txn);
    //     file->update_node(node, node->disk_location, sizeof(InternalNodeT), txn);
    //     file->update_node(left, left->disk_location, sizeof(InternalNodeT), txn);


    //     if(check_underflow(parent, txn) && parent->parent != 0) //if parent underflowed
    //     {

    //         internal_underflow(parent,txn);
    //     }
    //     if (parent->parent == 0 && parent->current_key_count == 0)
    //     {
    //         left->parent = 0;

    //         //off_t old_root = tree_root;
    //         off_t new_root = left->disk_location;

    //         //TODO: fixfile->update_root_pointer(table, old_root, new_root);
    //         //std::cout << "[ROOT PROMOTION] Empty root -> new root: " << new_root << "\n";
    //         //tree_root = new_root;
    //         //root_node = left; // Update cache just in case

    //         file->update_node(left, left->disk_location, sizeof(InternalNodeT), txn);

    //         return; // Important: Stop processing the dead parent
    //     }
    // }
    // else //merge right into us
    // {
    //     //std::cout << "merge internal right";
    //     //node->keys[node->current_key_count] = parent->keys[child_index];
    //     memcpy(node->keys[node->current_key_count], parent->keys[child_index], KeyLen);
    //     node->current_key_count++;
    //     InternalNodeT *right = static_cast<InternalNodeT*>(file->load_node<NodeT>(parent->children[child_index+1], txn));
    //     //first move rights childer
    //     for (int i = 0; i < right->current_key_count + 1; i++) {
    //         node->children[node->current_key_count + i] = right->children[i];
    //     }

    //     int total_keys = node->current_key_count + right->current_key_count;
    //     if (total_keys > MaxKeys) {
    //         std::cerr << "FATAL: Merge overflow. MaxKeys: " << MaxKeys
    //                 << " Attempting to store: " << total_keys << "\n";
    //         exit(1);
    //     }
    //     // Move right's keys into node
    //     int current_key_iterator = 0;
    //     while (right->current_key_count > 0) {
    //         memcpy(node->keys[node->current_key_count], right->keys[current_key_iterator], KeyLen);
    //         node->current_key_count++;
    //         current_key_iterator++;
    //         right->current_key_count--;
    //     }



    //     // FIX: re-parent ALL of node's children and persist each one
    //     for (int i = 0; i < node->current_key_count + 1; i++) {
    //         if (node->children[i]) {
    //             NodeT* child = file->load_node<NodeT>(node->children[i], txn);
    //             child->parent = node->disk_location;
    //             file->update_node(child, child->disk_location, sizeof(InternalNodeT), txn);
    //         }
    //     }
    //     delete_index_in_node(child_index, parent, child_index + 1, txn);
    //     parent->children[child_index] = node->disk_location;

    //     file->update_node(parent, parent->disk_location, sizeof(InternalNodeT), txn);
    //     file->update_node(node, node->disk_location, sizeof(InternalNodeT), txn);
    //     file->update_node(right, right->disk_location, sizeof(InternalNodeT), txn);

    //     if(check_underflow(parent,txn) && parent->parent != 0) //if parent underflowed
    //     {
    //         internal_underflow(parent,txn);
    //     }
    //     if (parent->parent == 0&& parent->current_key_count == 0)
    //     {
    //         node->parent = 0;

    //         //off_t old_root = tree_root;
    //         off_t new_root = node->disk_location;

    //         // TODO: include file->update_root_pointer(table, old_root, new_root);
    //         //std::cout << "[ROOT PROMOTION] Empty root -> new root: " << new_root << "\n";
    //         //tree_root = new_root;
    //         //root_node = node; // Update cache just in case

    //         file->update_node(node, node->disk_location, sizeof(InternalNodeT), txn);

    //         return; // Important: Stop processing the dead parent
    //     }
    // }
// }
// template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
// bool BtreePlus<NodeT, LeafNodeT, InternalNodeT>
// ::attempt_borrow_internal(InternalNodeT* self, InternalNodeT* parent, int self_child_index, Transaction& txn)
// {
//     if(self_child_index != 0) //if not the left most sibling
//     {
//         InternalNodeT* left = static_cast<InternalNodeT*>(file->load_node<NodeT>(parent->children[self_child_index-1], txn));
//         if(left->current_key_count-1 > get_underflow_amount())  //if left has a free key
//         {
//             //std::cout << "borrowing left internal\n";
//             borrow_left_internal(self,parent,left, self_child_index,txn);

//             return true;
//         }
//     }
//         //if borrow left failed/ cant
//     if(self_child_index != parent->current_key_count)
//     {
//         InternalNodeT* right = static_cast<InternalNodeT*>(file->load_node<NodeT>(parent->children[self_child_index+1], txn));
//         //check right
//         if(right->current_key_count-1 > get_underflow_amount())
//         {
//             //std::cout << "borrowing right internal\n";
//             borrow_right_internal(self,parent,right, self_child_index,txn);

//             return true;
//         }
//     }
//     return false;

// }
// template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
// void BtreePlus<NodeT, LeafNodeT, InternalNodeT>
// ::borrow_left_internal(InternalNodeT *self, InternalNodeT* parent, InternalNodeT* left, int child_index, Transaction& txn)
// {
    // int separator_index = child_index - 1;

    // // 1. First, capture the child we are moving BEFORE modifying 'left'
    // // The child to move is the LAST child of 'left'.
    // // Indices are 0..current_key_count. Last child is at index 'current_key_count'.
    // off_t child_to_move_loc = left->children[left->current_key_count];

    // // 2. Copy parent key to self (push_into_internal logic is complex, ensuring we do this right)
    // push_into_internal(self, parent->keys[separator_index], txn);

    // // 3. Update parent key with left's last key
    // memset(parent->keys[separator_index],0,KeyLen);
    // memcpy(parent->keys[separator_index], left->keys[left->current_key_count-1], KeyLen);

    // // 4. Remove the key and the child from left
    // // BUG WAS HERE: You were passing 'left->current_key_count-1' as the child index.
    // // That deleted the SECOND TO LAST child. We must delete the LAST child.
    // delete_index_in_node(left->current_key_count-1, left, left->current_key_count,txn);

    // // 5. Assign the captured child to self's first child slot
    // // push_into_internal shifted children right, so children[0] is free to overwrite?
    // // Actually, push_into_internal shifts children[0] to children[1].
    // // So children[0] is strictly the new slot.
    // self->children[0] = child_to_move_loc;

    // // 6. Update parent pointer of the moved child
    // NodeT* moved_child = file->load_node<NodeT>(child_to_move_loc, txn);
    // moved_child->parent = self->disk_location;
    // // left->children[...] = 0; // Handled by delete_index_in_node implicitly


    // file->update_node(moved_child, moved_child->disk_location, sizeof(InternalNodeT), txn);
    // file->update_node(left, left->disk_location, sizeof(InternalNodeT), txn);
    // file->update_node(parent, parent->disk_location, sizeof(InternalNodeT), txn);
// }
// template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
// void BtreePlus<NodeT, LeafNodeT, InternalNodeT>
// ::borrow_right_internal(InternalNodeT *self,InternalNodeT* parent, InternalNodeT* right, int child_index, Transaction& txn)
// {
    // //std::cout << "borrow right internal" << "\n";
    // int separator_index = child_index;
    // //first we copy parent key
    // //push_into_internal(self, parent->keys[separator_index]);
    // std::memcpy(self->keys[self->current_key_count],parent->keys[separator_index], KeyLen);
    // self->current_key_count++;
    // //now we replace the parent with the right's value,
    // memcpy(parent->keys[separator_index], right->keys[0], KeyLen);
    // //remove the right's key
    // self->children[self->current_key_count] = right->children[0];
    // NodeT* moved_child = file->load_node<NodeT>(right->children[0], txn);
    // moved_child->parent = self->disk_location;
    // right->children[0] = 0;

    // file->update_node(moved_child, moved_child->disk_location, sizeof(InternalNodeT), txn);

    // //std::cout << "\n val: " << right->children[1];
    // delete_index_in_node(0, right,0, txn);
    // //we forgot to change the moved nodes parent

    // file->update_node(self, self->disk_location, sizeof(InternalNodeT), txn);
    // file->update_node(right, right->disk_location, sizeof(InternalNodeT), txn);
    // file->update_node(parent, parent->disk_location, sizeof(InternalNodeT), txn);
    // //std::cout << "\n val: " << right->children[0];
// }
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::push_into_internal(InternalNodeT* target, char* value, Transaction& txn)
{
    // FIX: Start at current_key_count (the new index), not current_key_count-1
    for (int i = target->current_key_count; i > 0; i--)
    {
        memcpy(target->keys[i], target->keys[i-1], KeyLen);
    }

    memcpy(target->keys[0], value, KeyLen);

    // This loop was already correct
    for (int i = target->current_key_count; i >= 0; i--)
    {
        target->children[i + 1] = target->children[i];
    }
    target->current_key_count++;
    file->update_node(target, target->disk_location, sizeof(InternalNodeT), txn);
}

// template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
// bool BtreePlus<NodeT, LeafNodeT, InternalNodeT>
// ::check_underflow(NodeT* node, Transaction& txn)
// {
    // if (node->parent == 0) {
    //     // Root is allowed to have even 1 key (or 0 in some degenerate cases),
    //     // but almost never triggers underflow check — usually handled separately.
    //     // For safety we return false here (no underflow treatment for root).
    //     return false;
    // }

    // // Non-root node: underflow if keys < ⌈(MaxKeys+1)/2⌉ - 1
    // // which is equivalent to keys < (MaxKeys + 1) / 2    (integer division)
    // int min_keys_nonroot = MaxKeys / 2;

    //return (node->current_key_count < min_keys_nonroot);
//     return false;
// }
// template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
// int BtreePlus<NodeT, LeafNodeT, InternalNodeT>::get_underflow_amount()
// {
    // Logic: A node underflows if keys < (MaxKeys + 1) / 2
    // We can only borrow if (count - 1) >= (MaxKeys + 1) / 2
    // So trigger threshold is: count - 1 > ((MaxKeys + 1) / 2) - 1

//     int min_keys = (MaxKeys) / 2;
//     return min_keys - 1;
// }
// template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
// bool BtreePlus<NodeT, LeafNodeT, InternalNodeT>
// ::attempt_borrow(NodeT *current, InternalNodeT *parent, int currents_child_index, Transaction& txn)
// {
//     //check left
//     if(currents_child_index != 0)
//     {
//         NodeT* left = file->load_node<NodeT>(parent->children[currents_child_index-1], txn);
//         if(left->current_key_count-1 > get_underflow_amount())
//         {
//             //borrow left
//             borrow_left_leaf(current,left,txn);
//             //update separator
//             std::memcpy(parent->keys[currents_child_index-1], current->keys[0], KeyLen);
//             file->update_node(parent, parent->disk_location, sizeof(InternalNodeT), txn);

//             return true;
//         }
//     }
//     if(currents_child_index != parent->current_key_count)
//     {
//         NodeT* right = file->load_node<NodeT>(parent->children[currents_child_index+1], txn);
//         //check right
//         if(right->current_key_count-1 > get_underflow_amount())
//         {
//             borrow_right_leaf(current,right, txn);
//             std::memcpy(parent->keys[currents_child_index], right->keys[0], KeyLen);
//             file->update_node(parent, parent->disk_location, sizeof(InternalNodeT), txn);

//             return true;
//         }
//     }
//     return false;
// }
// template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
// void BtreePlus<NodeT, LeafNodeT, InternalNodeT>
// ::borrow_left_leaf(NodeT* current, NodeT* left, Transaction& txn)
// {
//     //std::cout << "borrow left leaf"<<"\n";
//     LeafNodeT* cur_cast  = static_cast<LeafNodeT*>(current);
//     LeafNodeT* left_cast = static_cast<LeafNodeT*>(left);

//     // STEP 1: Capture the key and value from left's last position
//     char borrowed_key[KeyLen];
//     std::memcpy(borrowed_key, left->keys[left->current_key_count - 1], KeyLen);
//     off_t borrowed_value = left_cast->values[left->current_key_count - 1];

//     // STEP 2: Shift everything in current to the right
//     for (int i = current->current_key_count; i > 0; i--) {
//         std::memcpy(current->keys[i], current->keys[i - 1], KeyLen);
//         cur_cast->values[i] = cur_cast->values[i - 1];
//     }

//     // STEP 3: Insert borrowed key/value at position 0
//     std::memcpy(current->keys[0], borrowed_key, KeyLen);
//     cur_cast->values[0] = borrowed_value;
//     current->current_key_count++;

//     // STEP 4: Remove from left
//     delete_index_in_node(left->current_key_count - 1, left, -1, txn);

//     std::fill(std::begin(left->keys[left->current_key_count]), std::end(left->keys[left->current_key_count]), 0);

//     static_cast<LeafNodeT*>(left)->values[left->current_key_count] = 0;

//     // Update on disk
//     file->update_node(current, current->disk_location, sizeof(LeafNodeT), txn);
//     file->update_node(left, left->disk_location, sizeof(LeafNodeT), txn);
// }

// template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
// void BtreePlus<NodeT, LeafNodeT, InternalNodeT>
// ::borrow_right_leaf(NodeT* current, NodeT* right, Transaction& txn)
// {
//     //std::cout << "borrow right leaf"<<"\n";
//     LeafNodeT* cur_cast  = static_cast<LeafNodeT*>(current);
//     LeafNodeT* right_cast = static_cast<LeafNodeT*>(right);

//     // STEP 1: Capture key and value from right[0]
//     char borrowed_key[KeyLen];
//     std::memcpy(borrowed_key, right->keys[0], KeyLen);
//     off_t borrowed_value = right_cast->values[0];

//     // STEP 2: Append to current (no shift needed, adding to end)
//     std::memcpy(current->keys[current->current_key_count], borrowed_key, KeyLen);
//     cur_cast->values[current->current_key_count] = borrowed_value;
//     current->current_key_count++;

//     // STEP 3: Remove from right (this shifts right's array left)
//     delete_index_in_node(0, right, -1,txn);

//     std::fill(std::begin(right->keys[right->current_key_count]), std::end(right->keys[right->current_key_count]), 0);
//     static_cast<LeafNodeT*>(right)->values[right->current_key_count] = 0;

//     // Update on disk
//     file->update_node(current, current->disk_location, sizeof(LeafNodeT), txn);
//     file->update_node(right, right->disk_location, sizeof(LeafNodeT), txn);
// }


template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
LeafNodeT* BtreePlus<NodeT, LeafNodeT, InternalNodeT>
::traverse_to_leaf(char* to_search, SharedPageGuard& current_guard, off_t start_location, Transaction& txn)
{   
    //current guard is already locked by caller, so we can safely load the first node without worrying about lock acquisition here
    NodeT* cursor = file->load_node<NodeT>(start_location, txn);

    while(!cursor->is_leaf)
    {
        InternalNodeT *cursor_cast = static_cast<InternalNodeT*>(cursor);

        off_t next_location = get_next_node_pointer(to_search, cursor_cast);

        current_guard = SharedPageGuard(next_location, txn);

        cursor = file->load_node<NodeT>(next_location, txn);
    }
    assert(cursor->is_leaf);
    return static_cast<LeafNodeT*>(cursor);
}


template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
int BtreePlus<NodeT, LeafNodeT, InternalNodeT>::find_child_index(InternalNodeT* parent, off_t child) {
    for (int i = 0; i < parent->current_key_count + 1; ++i) {
        if (parent->children[i] == child) // or pointer equality depending on design
            return i;
    }
    return -1; // should never happen if tree is consistent
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
off_t BtreePlus<NodeT, LeafNodeT, InternalNodeT>::get_next_node_pointer(char* to_insert, InternalNodeT* node)
{
    int left = 0;
    int right = node->current_key_count - 1;
    int result = node->current_key_count; // default: rightmost child

    while (left <= right) {
        int mid = left + (right - left) / 2;
        int cmp = memcmp(to_insert, node->keys[mid], KeyLen);

        if (cmp < 0) {
            result = mid;
            right = mid - 1;
        } else {
            // to_insert >= keys[mid], go right
            left = mid + 1;
        }
    }

    return node->children[result];
}

template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
int BtreePlus<NodeT, LeafNodeT, InternalNodeT>::get_first_key_index_gte(char* to_locate, LeafNodeT* node)
{

    int left = 0;
    int right = node->current_key_count - 1;
    int mid;

    while (left <= right)
    {
        mid = (left + right) / 2;
        int cmp = memcmp(to_locate, node->keys[mid], KeyLen);

        if (cmp <= 0)
            right = mid - 1;
        else
            left = mid + 1;
    }

    // left ends up at the first key greater than to_locate
    return left;
}


template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
off_t BtreePlus<NodeT, LeafNodeT, InternalNodeT>::search_recursive(char* search_key, InternalNodeT* node)
{
    int i = 0;

    // find first key strictly greater than search_key
    for (; i < node->current_key_count; i++)
    {
        if (std::memcmp(search_key, node->keys[i], KeyLen) <= 0)
            break;
    }




    return node->children[i];
}

template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
int BtreePlus<NodeT, LeafNodeT, InternalNodeT>::leaf_lower_bound(NodeT* leaf, const std::string& key)
{
    int left = 0;
    int right = leaf->current_key_count - 1;
    int result = leaf->current_key_count; // default: past the end

    // Prepare a fixed-width search key
    unsigned char search[KeyLen];
    memset(search, 0, KeyLen);
    memcpy(search, key.data(), std::min(key.size(), (size_t)KeyLen));

    while (left <= right)
    {
        int mid = left + (right - left) / 2;

        int cmp = memcmp(search, leaf->keys[mid], KeyLen);

        if (cmp <= 0)  // key ≤ leaf->keys[mid], go left
        {
            result = mid;
            right = mid - 1;
        }
        else
        {
            left = mid + 1;
        }
    }

    // result points to first key ≥ search, or current_key_count if none
    return (result == leaf->current_key_count) ? -1 : result;
}



template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
int BtreePlus<NodeT, LeafNodeT, InternalNodeT>::leaf_contains(NodeT* leaf,
                                                            const std::string& key)
{
    int left = 0;
    int right = leaf->current_key_count - 1;
    int result = -1;

    // Prepare a fixed-width search key
    unsigned char search[KeyLen];
    memset(search, 0, KeyLen);
    memcpy(search, key.data(), std::min(key.size(), (size_t)KeyLen));

    while (left <= right)
    {
        int mid = left + (right - left) / 2;

        // Compare exactly KeyLen bytes
        int cmp = memcmp(search, leaf->keys[mid], KeyLen);

        if (cmp == 0)
        {
            result = mid;
            right = mid - 1;   // find leftmost duplicate
        }
        else if (cmp < 0)
        {
            right = mid - 1;
        }
        else
        {
            left = mid + 1;
        }
    }

    return result;
}




template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>
::insert_up_into(Insert_Up_Data data,off_t node_location, Transaction& txn, Column& column, std::vector<off_t> &stack)
{
    NodeT *node = file->load_node<NodeT>(node_location, txn);
    insert_key_into_node(data,node);
    if(node->current_key_count == MaxKeys)
    {
        split_internal(node, txn, column, stack);
    }
    file->update_node(node,node->disk_location, sizeof(InternalNodeT), txn);
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>
::split_leaf(NodeT* node, Transaction& txn, Column& column, std::vector<off_t> &stack)
{
    LeafNodeT right_node = {};
    off_t right_node_location = file->alloc_block();
    right_node.disk_location = right_node_location;

    file->cache.read_block(right_node_location);
    txn.acquire_ownership_and_copy_if_needed(right_node_location);

    assert(node->is_leaf);
    LeafNodeT* node_cast = static_cast<LeafNodeT*>(node);
    LeafNodeT* right_node_cast = &right_node;
    right_node_cast->next_leaf = node_cast->next_leaf;
    node_cast->next_leaf = right_node_cast->disk_location;

    int middle_index = MaxKeys/2;
    char middle_key[KeyLen] = {0};
    std::memcpy(middle_key, node->keys[middle_index], KeyLen);

    char temp_keys[MaxKeys][KeyLen] = {0};
    std::memcpy(temp_keys, node->keys, sizeof(node->keys));
    for (int i = middle_index; i < MaxKeys; i++) {
        std::fill(std::begin(node->keys[i]), std::end(node->keys[i]), 0);
    }
    for (int i = middle_index; i < MaxKeys; i++) {
        std::memcpy(right_node_cast->keys[i - middle_index], temp_keys[i], KeyLen);  
    }
    off_t temp_values[MaxKeys] = {0};
    std::memcpy(temp_values, node_cast->values, sizeof(node_cast->values));
    for (int i = middle_index; i < MaxKeys; i++) {
        node_cast->values[i] = 0;
    }
    for (int i = middle_index; i < MaxKeys; i++) {
        right_node_cast->values[i - middle_index] = temp_values[i];
    }
    node->current_key_count = middle_index;
    right_node.current_key_count = MaxKeys - middle_index;  

    // --- NEW: ROOT CHECK VIA STACK ---
    if (stack.empty()) // if we are root
    {
        InternalNodeT new_parent = {};
        new_parent.disk_location = file->alloc_block();
        file->cache.read_block(new_parent.disk_location);

        txn.acquire_ownership_and_copy_if_needed(new_parent.disk_location);

        InternalNodeT* new_parent_cast = &new_parent;
        new_parent.is_leaf = false;
        new_parent_cast->children[0] = node->disk_location;
        new_parent_cast->children[1] = right_node.disk_location;

        // Update root pointer on disk and in memory
        file->update_root_pointer(column.indexLocation, new_parent.disk_location);
        column.indexLocation = new_parent.disk_location;

        std::memcpy(new_parent.keys[0], middle_key, KeyLen);
        new_parent.current_key_count = 1;
        
        file->update_node(static_cast<NodeT*>(static_cast<void*>(&right_node)), right_node.disk_location, sizeof(LeafNodeT), txn);
        file->update_node(node, node->disk_location, sizeof(LeafNodeT), txn);
        file->update_node(static_cast<NodeT*>(static_cast<void*>(&new_parent)), new_parent.disk_location, sizeof(InternalNodeT), txn);
    }
    else
    {
        Insert_Up_Data data = {};
        memcpy(data.key, middle_key, KeyLen);
        data.left_child = node->disk_location;
        data.right_child = right_node.disk_location;
        
        file->update_node(static_cast<NodeT*>(static_cast<void*>(&right_node)), right_node.disk_location, sizeof(LeafNodeT), txn);
        file->update_node(node, node->disk_location, sizeof(LeafNodeT), txn);

        // --- NEW: POP PARENT FROM STACK ---
        off_t parent_location = stack.back();
        stack.pop_back();

        insert_up_into(data, parent_location, txn, column, stack);
    }
}

template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::split_internal(
    NodeT* node, 
    Transaction& txn, 
    Column& column, 
    std::vector<off_t> &stack)
{
    InternalNodeT right_node = {};
    off_t right_node_location = file->alloc_block();
    right_node.disk_location = right_node_location;

    file->cache.read_block(right_node_location);
    txn.acquire_ownership_and_copy_if_needed(right_node_location);

    right_node.is_leaf = false;
    // right_node.parent = node->parent; <-- REMOVED

    int middle_index = MaxKeys/2;
    char middle_key[KeyLen] = {0};
    std::memcpy(middle_key, node->keys[middle_index], KeyLen);

    char temp_keys[MaxKeys][KeyLen] = {0};
    std::memcpy(temp_keys, node->keys, sizeof(node->keys));
    for (int i = middle_index; i < MaxKeys; i++) {
        std::fill(std::begin(node->keys[i]), std::end(node->keys[i]), 0);
    }
    
    InternalNodeT* right_cast = &right_node;
    for (int i = middle_index+1; i < MaxKeys; i++) {
        std::memcpy(right_cast->keys[i - middle_index-1], temp_keys[i], KeyLen);
    }
    
    node->current_key_count = middle_index;
    right_node.current_key_count = MaxKeys - middle_index - 1;
    InternalNodeT* node_cast = static_cast<InternalNodeT*>(node);

    // --- MASSIVE OPTIMIZATION HERE ---
    // We only move the child pointers. We NO LONGER load the children 
    // to update their parent pointers, because they don't have any!
    for (int i = middle_index + 1; i <= MaxKeys; i++) {
        right_cast->children[i - (middle_index + 1)] = node_cast->children[i];
        node_cast->children[i] = 0;
    }

    // --- NEW: ROOT CHECK VIA STACK ---
    if (stack.empty()) // If we are the root
    {
        InternalNodeT new_parent = {};
        off_t new_parent_location = file->alloc_block();
        new_parent.disk_location = new_parent_location;

        file->cache.read_block(new_parent_location);
        txn.acquire_ownership_and_copy_if_needed(new_parent_location);

        // Update root pointer on disk and in memory
        file->update_root_pointer(column.indexLocation, new_parent.disk_location);
        column.indexLocation = new_parent.disk_location;

        InternalNodeT* new_parent_cast = &new_parent;
        new_parent_cast->children[0] = node->disk_location;
        new_parent_cast->children[1] = right_node.disk_location;
        new_parent.is_leaf = false;
        new_parent.current_key_count = 1;
        
        std::memcpy(new_parent.keys[0], middle_key, KeyLen);
        
        file->update_node(static_cast<NodeT*>(static_cast<void*>(&new_parent)), new_parent_location, sizeof(InternalNodeT), txn);
        file->update_node(node, node->disk_location, sizeof(InternalNodeT), txn);
        file->update_node(static_cast<NodeT*>(static_cast<void*>(&right_node)), right_node.disk_location, sizeof(InternalNodeT), txn);
    }
    else
    {
        Insert_Up_Data data = {};
        std::memcpy(data.key, middle_key, KeyLen);

        data.left_child = node->disk_location;
        data.right_child = right_node.disk_location;
        
        file->update_node(static_cast<NodeT*>(static_cast<void*>(&right_node)), right_node_location, sizeof(InternalNodeT), txn);
        file->update_node(node, node->disk_location, sizeof(InternalNodeT), txn);
        
        // --- NEW: POP PARENT FROM STACK ---
        off_t parent_location = stack.back();
        stack.pop_back();

        insert_up_into(data, parent_location, txn, column, stack);
    }
}

template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>
::insert_key_into_node(Insert_Up_Data data, NodeT* node)
{
    int insert_positon = 0;
    while (insert_positon < node->current_key_count && is_less_than(node->keys[insert_positon],data.key)) {
        insert_positon++;
    }

    // Shift elements to the right to make space
    for (int i = node->current_key_count; i > insert_positon; i--) {
        std::memcpy(node->keys[i], node->keys[i-1], KeyLen);
    }
    //if is an internal node, shift children right too
    if(!node->is_leaf)
    {
        InternalNodeT *node_cast = static_cast<InternalNodeT*>(node);                                                                                            //insert 3, z, i    //position = 2
        for (int i = node->current_key_count+1; i > insert_positon; i--)
        {                                                                                                     //keys[1,2,3,4,0,0] //children[x,y,z,t,0,0] // x y z   t 0 i = 3
            node_cast->children[i] = node_cast->children[i - 1];
        }
        node_cast->children[insert_positon + 1] = data.right_child;
    }
    else            //if is leaf, shift values too
    {
        LeafNodeT *node_cast = static_cast<LeafNodeT*>(node);
        for (int i = node->current_key_count; i > insert_positon; i--) {
            node_cast->values[i] = node_cast->values[i - 1];
        }
        node_cast->values[insert_positon] = data.right_child;
    }
    // Insert the new key
    std::memcpy(node->keys[insert_positon], data.key, KeyLen);

    node->current_key_count++;
}
// template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
// void BtreePlus<NodeT, LeafNodeT, InternalNodeT>
// ::delete_index_in_node(int index, NodeT* node, int child_index, Transaction& txn)
// {
//     // Guard against deleting from empty node
//     if(node->current_key_count == 0) {
//         std::cout << "Error: Attempted to delete from an empty node.\n";
//         return;
//     }

//     // Shift keys left
//     for (int i = index; i < node->current_key_count-1; i++) {
//         std::memcpy(node->keys[i], node->keys[i+1], KeyLen);
//     }

//     if(node->is_leaf)
//     {
//         LeafNodeT* node_cast = static_cast<LeafNodeT*>(node);
//         for (int i = index; i < node->current_key_count-1; i++)
//         {
//             node_cast->values[i] = node_cast->values[i+1];
//         }
//         node_cast->values[node->current_key_count-1] = 0;
//     }

//     std::fill(std::begin(node->keys[node->current_key_count - 1]),
//               std::end(node->keys[node->current_key_count - 1]), 0);
//     node->current_key_count--;

//     if(!node->is_leaf && child_index >= 0)
//     {
//         InternalNodeT* node_cast = static_cast<InternalNodeT*>(node);
//         for (int i = child_index; i <= node->current_key_count; i++)
//         {
//             node_cast->children[i] = node_cast->children[i+1];
//         }
//     }
// }
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
LeafNodeT* BtreePlus<NodeT, LeafNodeT, InternalNodeT>::find_leftmost_leaf(off_t root_location, Transaction& txn) {

    NodeT* curr = file->load_node<NodeT>(root_location, txn);
    while (!curr->is_leaf) {
        InternalNodeT* in = static_cast<InternalNodeT*>(curr);
        curr = file->load_node<NodeT>(in->children[0], txn); // go all the way left
    }
    return static_cast<LeafNodeT*>(curr);
}

// Explicit instantiations
template class BtreePlus<Node32, LeafNode32, InternalNode32>;
template class BtreePlus<Node16, LeafNode16, InternalNode16>;
template class BtreePlus<Node8, LeafNode8, InternalNode8>;
template class BtreePlus<Node4, LeafNode4, InternalNode4>;
