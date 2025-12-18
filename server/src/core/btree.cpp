#include "btree.hpp"
#include <type_traits> 

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
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::insert(std::string insert_string, off_t &record_location)
{
    //stirng to c_str
    char buffer[KeyLen] = {0};
    std::memcpy(buffer, insert_string.c_str(), insert_string.length());
    char to_insert[KeyLen] = {0};
    std::memcpy(to_insert, buffer, KeyLen);
   
    NodeT* cursor = root_node;
                  
   
   
    while(!cursor->is_leaf)
    {
        InternalNodeT *cursor_cast = static_cast<InternalNodeT*>(cursor);
        cursor = file->load_node<NodeT>(get_next_node_pointer(to_insert,cursor_cast));
    }



    //insert into current node
    Insert_Up_Data data = {};
    memset(data.key, 0, KeyLen);
    memcpy(data.key,to_insert,KeyLen);
    data.left_child = 0;

    data.right_child = record_location;
    insert_key_into_node(data,cursor);

   
    file->update_node(cursor, cursor->disk_location, sizeof(InternalNodeT));
    if(cursor->current_key_count == MaxKeys)
    {
        split_leaf(cursor);
    }

}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::delete_key(std::string delete_string)
{
    char buffer[KeyLen];
    std::memcpy(buffer, delete_string.c_str(), KeyLen);
    char to_delete[KeyLen];
    std::memcpy(to_delete, buffer, KeyLen);
    root_node = file->load_node<NodeT>(0); //TODO: NO MULT TABLE SUPPORT
    if(root_node == nullptr) perror("error no root");
    NodeT* cursor = root_node;
    while(!cursor->is_leaf)
    {
        InternalNodeT *cursor_cast = static_cast<InternalNodeT*>(cursor);
        cursor = file->load_node<NodeT>(get_next_node_pointer(to_delete,cursor_cast));
    }
    print_tree();
    //here we reached the leaf node
    //remove from leafnode
    for (int i = 0; i < cursor->current_key_count; i++)
    {
        if(std::strcmp(cursor->keys[i], to_delete) == 0)        //TODO: BROKEN ON INTS
        {
            //found
            std::cout << "index: " << i;
            delete_index_in_node(i,cursor, false);
            bool borrow_success = false;
            file->update_node(cursor, cursor->disk_location, sizeof(InternalNodeT));
            if(check_underflow(cursor))//check if leaf underflowd
            {
                NodeT *parent = file->load_node<NodeT>(cursor->parent);
                InternalNodeT* parent_cast = static_cast<InternalNodeT*>(parent);
                int child_index = find_child_index(parent_cast, cursor->disk_location);
                //try borrow
                borrow_success = attempt_borrow(cursor,parent_cast, child_index);
                if(!borrow_success)
                {
                    //hanlde leaf merge
                    leaf_merge(parent_cast,cursor, child_index);
                }
            }
            //check if it was the first key of a leaf
            if(i == 0 && borrow_success) //if first key was removed
            {
                NodeT *parent = file->load_node<NodeT>(cursor->parent);
                InternalNodeT* parent_cast = static_cast<InternalNodeT*>(parent);
                int child_index = find_child_index(parent_cast, cursor->disk_location);
                std::cout << "parent index: " << child_index;
                if(child_index != 0)    //if not the first child
                {
                    //replace the parents key
                    memcpy(parent_cast->keys[child_index-1],cursor->keys[0], KeyLen);
                    //if(cursor->current_key_count == 0) parent_cast->current_key_count = 0;
                    file->update_node(parent_cast, parent_cast->disk_location, sizeof(InternalNodeT));
                }
            }
           
            //print_tree();
            break;
        }
    }
   
   
   
}

template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
LocationData<LeafNodeT> BtreePlus<NodeT, LeafNodeT, InternalNodeT>::locate(std::string key)
{
    LocationData<LeafNodeT> output;

    char buffer[KeyLen] = {0};
    std::memcpy(buffer, key.c_str(), key.length());
    char to_insert[KeyLen] = {0};
    std::memcpy(to_insert, buffer, KeyLen);
   
    NodeT* cursor = root_node;
   
    while(!cursor->is_leaf)
    {
        InternalNodeT *cursor_cast = static_cast<InternalNodeT*>(cursor);
        cursor = file->load_node<NodeT>(get_next_node_pointer(to_insert,cursor_cast));
    }

    off_t index = leaf_contains(cursor, key);

    if (index == -1) return output;

    output.leaf = static_cast<LeafNodeT*>(cursor);
    output.index = index;


    return output;
}


template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
std::vector<off_t> BtreePlus<NodeT, LeafNodeT, InternalNodeT>::search(std::string search_string)
{
    std::vector<off_t> output;
    char search_key[KeyLen] = {0};
    std::memcpy(search_key, search_string.c_str(), search_string.length()); // keep strncpy

    root_node = file->load_node<NodeT>(tree_root);
    NodeT* cursor = root_node;

    while (!cursor->is_leaf)
    {
        InternalNodeT* cursor_cast = static_cast<InternalNodeT*>(cursor);
        cursor = file->load_node<NodeT>(search_recursive(search_key, cursor_cast));
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
            leaf = static_cast<LeafNodeT*>(file->load_node<NodeT>(leaf->next_leaf));
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
bool BtreePlus<NodeT, LeafNodeT, InternalNodeT>::has_key(const std::string &key)
{
    char buffer[KeyLen] = {0};
    std::memcpy(buffer, key.c_str(), key.length());
    char to_insert[KeyLen] = {0};
    std::memcpy(to_insert, buffer, KeyLen);
   
    NodeT* cursor = root_node;
   
    while(!cursor->is_leaf)
    {
        InternalNodeT *cursor_cast = static_cast<InternalNodeT*>(cursor);
        cursor = file->load_node<NodeT>(get_next_node_pointer(to_insert,cursor_cast));
    }

    if (leaf_contains(cursor, key) != -1) { return true; }

    return false;
}



template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::print_tree()
{
    root_node = file->load_node<NodeT>(tree_root);
    std::cout << "B+ Tree structure (root at " << root_node->disk_location << "):\n";
    std::ofstream fout("btree_dump.txt");
    print_recursive(root_node, 0, fout);
    fout.close();
}


template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::init_root(off_t &location)
{
    NodeT *firstNode = file->load_node<NodeT>(location);
    firstNode->is_leaf = true;
    firstNode->disk_location = location;

    file->update_node<NodeT>(firstNode, location, sizeof(InternalNodeT));
    root_node = file->load_node<NodeT>(location);
    tree_root = location;

}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::leaf_merge(InternalNodeT* parent,NodeT* current, int child_index)
{
    if(child_index != 0)    //merge to left
    {
        //take out keys and vals
        //and apped them onto child_index-1
        NodeT *left = file->load_node<NodeT>(parent->children[child_index-1]);
        LeafNodeT *left_cast = static_cast<LeafNodeT*>(left);
        int current_key_iterator = 0;
        //move elements here
        while(current->current_key_count > 0)
        {
            memcpy(left_cast->keys[left_cast->current_key_count], current->keys[current_key_iterator], KeyLen);
            left_cast->current_key_count++;
            current_key_iterator++;
            current->current_key_count--;
        }
        //after its done we will remove the separator in the parent
        int separator_index = child_index -1;
        delete_index_in_node(separator_index, parent, child_index);
        
        
        //here we will recursivly check for the underflow in the internal node
        if(check_underflow(parent)) //if parent underflowed
        {
            internal_underflow(parent);
        }
    }
    else                    //right merges with us
    {
        NodeT *right = file->load_node<NodeT>(parent->children[child_index+1]);
        //LeafNodeT *right_cast = static_cast<LeafNodeT*>(right);
        int current_key_iterator = 0;
        //move elements here
        while(right->current_key_count > 0)
        {
            memcpy(current->keys[current->current_key_count], right->keys[current_key_iterator], KeyLen);
            current->current_key_count++;
            current_key_iterator++;
            right->current_key_count--;
        }
        //after its done we will remove the separator in the parent
        int separator_index = child_index;
        delete_index_in_node(separator_index, parent, child_index);
       
        if(check_underflow(parent)) //if parent underflowed
        {
            internal_underflow(parent);
           
        }
    }  
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::internal_underflow(NodeT* node)
{
    InternalNodeT *node_cast = static_cast<InternalNodeT*>(node);
    InternalNodeT* parent = static_cast<InternalNodeT*>(file->load_node<NodeT>(node_cast->parent));
   
    int child_index = find_child_index(parent,node_cast->disk_location);
    //attempt borrow
    bool borrow_result = attempt_borrow_internal(node_cast, parent, child_index);
    if(borrow_result == false && parent->parent != 0)
    {
        //borrow failed, we must merge internal nodes
        merge_internal(node_cast, parent, child_index);
    }
   
   
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::merge_internal(InternalNodeT* node, InternalNodeT* parent, int child_index)
{
    //first we insert parent key into the back of this one
    if(child_index != 0) //merge with left
    {
        std::cout << "merge left";
        int separator_index = child_index-1;
        InternalNodeT *left = static_cast<InternalNodeT*>(file->load_node<NodeT>(parent->children[child_index-1]));
       
        memcpy(left->keys[left->current_key_count], parent->keys[separator_index],KeyLen);
        left->current_key_count++;
        //move nodes children into left
        int left_key_count = left->current_key_count;
        for(int i = 0; i < node->current_key_count + 1; i++) {
            left->children[left_key_count + i] = node->children[i];
            NodeT* child = file->load_node<NodeT>(node->children[i]);
            child->parent = left->disk_location;
            file->update_node(child, child->disk_location, sizeof(InternalNodeT));
        }
        for(int i = 0; i < node->current_key_count+left->current_key_count+1; i++)
        {
            NodeT* child = file->load_node<NodeT>(left->children[i]);
            std::cout << "\ndebug here \n\n" <<left->children[i] << std::endl;
            //std::cout << static_cast<long long>(node->children[i]) << "\n";
            child->parent = 10;
        }
        int current_key_iterator = 0;
        //move all keys to the end of the internal node
        while(node->current_key_count > 0)
        {
            memcpy(left->keys[left->current_key_count], node->keys[current_key_iterator], KeyLen);
            left->current_key_count++;
            current_key_iterator++;
            node->current_key_count--;
        }
        delete_index_in_node(separator_index,parent,separator_index);
        parent->children[child_index-1] = left->disk_location;
        if(check_underflow(parent) && parent->disk_location != 0) //if parent underflowed
        {
            //TODO: NO MULT TABLE SUPPORT
            internal_underflow(parent);
        }
        if(root_node->current_key_count == 0)
        {
            tree_root = node->disk_location;
            //file->update_root_pointer();    
            root_node = node;
           
        }
    }
    else //merge right into us
    {
        std::cout << "merge internal right";
        //node->keys[node->current_key_count] = parent->keys[child_index];
        memcpy(node->keys[node->current_key_count], parent->keys[child_index], KeyLen);
        node->current_key_count++;
        InternalNodeT *right = static_cast<InternalNodeT*>(file->load_node<NodeT>(parent->children[child_index+1]));
        //first move rights childer
        for(int i = 0; i < right->current_key_count+1; i++)
        {
            node->children[node->current_key_count+i] = right->children[i];
           
        }
        for(int i = 0; i < right->current_key_count+right->current_key_count+1; i++)
        {
            NodeT* child = file->load_node<NodeT>(node->children[i]);
            std::cout << "\ndebug here \n\n" <<node->children[i] << std::endl;
            //std::cout << static_cast<long long>(node->children[i]) << "\n";
            child->parent = node->disk_location;
        }
        int current_key_iterator = 0;
        //move all keys to the end of the internal node
        while(right->current_key_count > 0)
        {
            memcpy(node->keys[node->current_key_count], right->keys[current_key_iterator], KeyLen);
            node->current_key_count++;
            current_key_iterator++;
            right->current_key_count--;
        }
        delete_index_in_node(child_index,parent,child_index);
        parent->children[child_index] = node->disk_location;
       //TODO: NO MULT TABLE SUPPORT
        if(check_underflow(parent) && parent->disk_location != 0) //if parent underflowed
        {
            internal_underflow(parent);
        }
        if(root_node->current_key_count == 0)
        {
            tree_root = node->disk_location;
            //file->update_root_pointer();    
            root_node = node;
           
        }
       
    }
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
bool BtreePlus<NodeT, LeafNodeT, InternalNodeT>::attempt_borrow_internal(InternalNodeT* self, InternalNodeT* parent, int self_child_index)
{
    if(self_child_index != 0) //if not the left most sibling
    {
        InternalNodeT* left = static_cast<InternalNodeT*>(file->load_node<NodeT>(parent->children[self_child_index-1]));
        if(left->current_key_count-1 > get_underflow_amount())  //if left has a free key
        {
            borrow_left_internal(self,parent,left, self_child_index);
           
            return true;
        }
    }
        //if borrow left failed/ cant
    if(self_child_index != parent->current_key_count)
    {
        InternalNodeT* right = static_cast<InternalNodeT*>(file->load_node<NodeT>(parent->children[self_child_index+1]));
        //check right
        if(right->current_key_count-1 > get_underflow_amount())
        {
            borrow_right_internal(self,parent,right, self_child_index);
           
            return true;
        }
    }
    return false;
   
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::borrow_left_internal(InternalNodeT *self,InternalNodeT* parent, InternalNodeT* left, int child_index)
{
    int separator_index = child_index - 1;
    //first we copy parent key
    push_into_internal(self, parent->keys[separator_index]);
    //now we replace the parent with the left's value,
    memset(parent->keys[separator_index],0,KeyLen);
    memcpy(parent->keys[separator_index], left->keys[left->current_key_count-1], KeyLen);
    //remove the left's key
    delete_index_in_node(left->current_key_count-1, left, left->current_key_count-1);
    self->children[0] = left->children[left->current_key_count+1];
    NodeT* moved_child = file->load_node<NodeT>(left->children[left->current_key_count+1]);
    moved_child->parent = self->disk_location;
    left->children[left->current_key_count+1] = 0;
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::borrow_right_internal(InternalNodeT *self,InternalNodeT* parent, InternalNodeT* right, int child_index)
{
    int separator_index = child_index;
    //first we copy parent key
    //push_into_internal(self, parent->keys[separator_index]);
    std::memcpy(self->keys[self->current_key_count],parent->keys[separator_index], KeyLen);
    self->current_key_count++;
    //now we replace the parent with the right's value,
    memcpy(parent->keys[separator_index], right->keys[0], KeyLen);
    //remove the right's key
    self->children[self->current_key_count] = right->children[0];
    NodeT* moved_child = file->load_node<NodeT>(right->children[0]);
    moved_child->parent = self->disk_location;
    right->children[0] = 0;
   
    std::cout << "\n val: " << right->children[1];
    delete_index_in_node(0, right,0);
    //we forgot to change the moved nodes parent
   
    std::cout << "\n val: " << right->children[0];
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::push_into_internal(InternalNodeT* target, char* value)
{
    std::cout << "value: " << value << '\n';
    for (int i = target->current_key_count-1; i > 0; i--)
    {
        memcpy(target->keys[i], target->keys[i-1], KeyLen);
    }
    memcpy(target->keys[0], value, KeyLen);
    for (int i = target->current_key_count; i >= 0; i--)
    {
        target->children[i + 1] = target->children[i];
    }
    target->current_key_count++;
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::print_recursive(NodeT* node, int depth, std::ostream& out)
{
    if (!node) return;
    std::string indent(depth * 4, ' ');

    if (node->is_leaf)
    {
        LeafNodeT* leaf = static_cast<LeafNodeT*>(node);
        out << indent << "Leaf Node (" << node->disk_location << ") [keys=" 
            << node->current_key_count << "]: ";
        for (int i = 0; i < node->current_key_count; ++i)
        {
            out << "[" << std::string(node->keys[i], KeyLen) << "]";
            if (i < node->current_key_count - 1) out << ", ";
        }
        out << " | next_leaf=" << leaf->next_leaf 
            << " Parent: " << node->parent << "\n";
    }
    else
    {
        InternalNodeT* in = static_cast<InternalNodeT*>(node);
        out << indent << "Internal Node (" << node->disk_location 
            << ") [keys=" << node->current_key_count << "]: ";
        for (int i = 0; i < node->current_key_count; ++i)
        {
            out << "[" << std::string(node->keys[i], KeyLen) << "]";
            if (i < node->current_key_count - 1) out << ", ";
        }
        out << node->parent << "\n";
        for (int i = 0; i <= node->current_key_count; ++i)
        {
            if (in->children[i] != 0)
            {
                NodeT* child = file->load_node<NodeT>(in->children[i]);
                print_recursive(child, depth + 1, out);
            }
        }
    }
}

template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
bool BtreePlus<NodeT, LeafNodeT, InternalNodeT>::check_underflow(NodeT* node)
{
    int x = (((MaxKeys-1)/2) - 1);
    return ((node->current_key_count) <= x);
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
int BtreePlus<NodeT, LeafNodeT, InternalNodeT>::get_underflow_amount()
{
    int x = (((MaxKeys-1)/2) - 1);
    return x;
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
bool BtreePlus<NodeT, LeafNodeT, InternalNodeT>::attempt_borrow(NodeT *current, InternalNodeT *parent, int currents_child_index)
{
    //check left
    if(currents_child_index != 0)
    {
        NodeT* left = file->load_node<NodeT>(parent->children[currents_child_index-1]);
        if(left->current_key_count-1 > get_underflow_amount())
        {
            //borrow left
            borrow_left_leaf(current,left);
            //update separator
            std::memcpy(parent->keys[currents_child_index-1], current->keys[0], KeyLen);
           
            return true;
        }
    }
    if(currents_child_index != parent->current_key_count)
    {
        NodeT* right = file->load_node<NodeT>(parent->children[currents_child_index+1]);
        //check right
        if(right->current_key_count-1 > get_underflow_amount())
        {
            borrow_right_leaf(current,right);
            std::memcpy(parent->keys[currents_child_index], right->keys[0], KeyLen);
           
            return true;
        }
    }
    return false;
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::borrow_left_leaf(NodeT* current, NodeT* left)
{
    std::memcpy(current->keys[0],left->keys[left->current_key_count-1], KeyLen);
    current->current_key_count++;
    delete_index_in_node(left->current_key_count-1,left,-1 );
    //TODO: also move the values pointers of the moved key
    if(!current->is_leaf) return;
    LeafNodeT* current_cast = static_cast<LeafNodeT*>(current);
    LeafNodeT* left_cast = static_cast<LeafNodeT*>(left);
    left_cast->values[left->current_key_count] = 0;
    off_t val = left_cast->values[left->current_key_count];       //NOT TESTED YET
    current_cast->values[0] = val;
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::borrow_right_leaf(NodeT* current, NodeT* right)
{
    std::memcpy(current->keys[current->current_key_count],right->keys[0], KeyLen);
    current->current_key_count++;
    delete_index_in_node(0,right, -1);
    if(!current->is_leaf) return;
    LeafNodeT* current_cast = static_cast<LeafNodeT*>(current);
    LeafNodeT* right_cast = static_cast<LeafNodeT*>(right);
    right_cast->values[0] = 0;
    off_t val = right_cast->values[0];       //NOT TESTED YET
    current_cast->values[current->current_key_count] = val;
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
    int mid;

    while (left <= right)
    {
        mid = (left + right) / 2;
        int cmp = memcmp(to_insert, node->keys[mid], KeyLen);

        if (cmp < 0)
            right = mid - 1;
        else
            left = mid + 1;
    }

    // left ends up at the first key greater than to_insert
    return node->children[left];
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
int BtreePlus<NodeT, LeafNodeT, InternalNodeT>::leaf_contains(NodeT* leaf, const std::string& key)
{
    int left = 0;
    int right = leaf->current_key_count - 1;

    while (left <= right)
    {
        int mid = left + (right - left) / 2;
        const std::string& mid_key = leaf->keys[mid];

        // Binary-safe compare
        size_t cmp_len = std::min(key.size(), mid_key.size()); // KeyLen = fixed length of node keys
        int cmp = memcmp(key.data(), mid_key.data(), cmp_len);

        if (cmp == 0)
        {
            // If prefixes equal, check length for total equality
            if (key.size() == mid_key.size())// key fits in fixed-size slot
                return mid;

            // Longer key sorts after shorter key
            cmp = (key.size() < mid_key.size()) ? -1 : 1;
        }

        if (cmp < 0)
            right = mid - 1;
        else
            left = mid + 1;
    }

    return -1; // not found
}


template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
int BtreePlus<NodeT, LeafNodeT, InternalNodeT>::find_left_node_child_index(NodeT *node)
{
    NodeT* loaded_parent = file->load_node<NodeT>(node->parent);
    InternalNodeT* parent = static_cast<InternalNodeT*>(loaded_parent);
    for (int i = 0; i < MaxKeys +1; i++)
    {
        if(parent->children[i] == node->disk_location)
        {
            return i;
        }
    }
    return -1;
   
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::insert_up_into(Insert_Up_Data data,off_t node_location)
{
    NodeT *node = file->load_node<NodeT>(node_location);
    insert_key_into_node(data,node);
    if(node->current_key_count == MaxKeys)
    {
        split_internal(node);
    }
    file->update_node(node,node->disk_location, sizeof(InternalNodeT));
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::split_leaf(NodeT* node)
{
    NodeT* right_node = new LeafNodeT();
    off_t right_node_location = file->alloc_block();
    //std::cout << "alloced block : " << right_node_location;
    right_node->disk_location = right_node_location;
    assert(node->is_leaf);
    LeafNodeT* node_cast = static_cast<LeafNodeT*>(node);
    LeafNodeT* right_node_cast = static_cast<LeafNodeT*>(right_node);
    right_node_cast->next_leaf = node_cast->next_leaf;
    node_cast->next_leaf = right_node_cast->disk_location;
   
    int middle_index = MaxKeys/2;
    char middle_key[KeyLen] = {0};  // ACTUAL ARRAY to store the key value
    std::memcpy(middle_key, node->keys[middle_index], KeyLen);  // COPY the value
    right_node->parent = node->parent;

    char temp_keys[MaxKeys][KeyLen] = {0};
    std::memcpy(temp_keys, node->keys, sizeof(node->keys));
    for (int i = middle_index; i < MaxKeys; i++) {
        std::fill(std::begin(node->keys[i]), std::end(node->keys[i]), 0);
    }
    //for (int i = 0; i < MaxKeys; i++) {
    //    //std::fill(std::begin(right_node->keys[i]), std::end(right_node->keys[i]), 0);
   // }
    for (int i = middle_index; i < MaxKeys; i++) {
        std::memcpy(right_node->keys[i - middle_index], temp_keys[i], KeyLen);
    }  
    // make a temp copy of the values before zeroing
    off_t temp_values[MaxKeys] = {0};
    std::memcpy(temp_values, node_cast->values, sizeof(node_cast->values));
    // clear out values in the left node after middle_index
    for (int i = middle_index; i < MaxKeys; i++) {
        node_cast->values[i] = 0;
    }
    // move the right half into the right_node
    for (int i = middle_index; i < MaxKeys; i++) {
        right_node_cast->values[i - middle_index] = temp_values[i];
    }
    node->current_key_count = middle_index;
    right_node->current_key_count = MaxKeys - middle_index;
    if (node->parent == 0)
    {
        NodeT* new_parent = new InternalNodeT();
        new_parent->disk_location = file->alloc_block();
        InternalNodeT* new_parent_cast = static_cast<InternalNodeT*>(new_parent);
        new_parent->is_leaf = false;
        new_parent_cast->children[0] = node->disk_location;
        new_parent_cast->children[1] = right_node->disk_location;

        node->parent = new_parent->disk_location;
        right_node->parent = new_parent->disk_location;

        root_node = new_parent;
        tree_root = new_parent->disk_location;
        //file->update_root_pointer();

        std::memcpy(new_parent->keys[0], middle_key, KeyLen);
        new_parent->current_key_count = 1;            

        file->update_node(right_node,right_node->disk_location, sizeof(InternalNodeT));
        file->update_node(node,node->disk_location, sizeof(InternalNodeT));
        file->update_node(new_parent,new_parent->disk_location, sizeof(InternalNodeT));


        delete new_parent;
    }
    else
    {
        Insert_Up_Data data = {};
        memcpy(data.key,middle_key,KeyLen);
        data.left_child = node->disk_location;
        data.right_child = right_node->disk_location;
        file->update_node(right_node,right_node->disk_location, sizeof(InternalNodeT));
        file->update_node(node,node->disk_location, sizeof(InternalNodeT));
       
        insert_up_into (data,node->parent);
    }
    delete right_node;
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::split_internal(NodeT* node)
{
    NodeT* right_node = new InternalNodeT();
    off_t right_node_location = file->alloc_block();
    right_node->disk_location = right_node_location;
    right_node->is_leaf = false;
    right_node->parent = node->parent;
    int middle_index = MaxKeys/2;
    char middle_key[KeyLen] = {0};  // ACTUAL ARRAY to store the key value
    std::memcpy(middle_key, node->keys[middle_index], KeyLen);  // COPY the value

    char temp_keys[MaxKeys][KeyLen] = {0};
    std::memcpy(temp_keys, node->keys, sizeof(node->keys)); // copy all keys
    // Zero out the right half of the left node
    for (int i = middle_index; i < MaxKeys; i++) {
        std::fill(std::begin(node->keys[i]), std::end(node->keys[i]), 0);
    }
    //for (int i = 0; i < MaxKeys; i++) {
    //    std::fill(std::begin(right_node->keys[i]), std::end(right_node->keys[i]), 0);
    //}
    // Copy the upper half into the right node
    InternalNodeT* right_leaf = static_cast<InternalNodeT*>(right_node);
    for (int i = middle_index+1; i < MaxKeys; i++) {
        std::memcpy(right_leaf->keys[i - middle_index-1], temp_keys[i], KeyLen);
    }
    node->current_key_count = middle_index;
    right_node->current_key_count = MaxKeys - middle_index - 1;
    // handle children pointers
    InternalNodeT* node_cast = static_cast<InternalNodeT*>(node);
    InternalNodeT* right_cast = static_cast<InternalNodeT*>(right_node);
    // left node keeps first (middle_index + 1) children
    // right node gets remaining children
   
    for (int i = middle_index + 1; i <= MaxKeys; i++) {
        right_cast->children[i - (middle_index + 1)] = node_cast->children[i];
       
        if (node_cast->children[i]) {  // FIXED: Update parent pointer
            NodeT* child = file->load_node<NodeT>(node_cast->children[i]);
            child->parent = right_node->disk_location;
            file->update_node(child,child->disk_location, sizeof(InternalNodeT));
        }
        node_cast->children[i] = 0;
    }
    if (node->parent != 0)
    {
        Insert_Up_Data data = {};
        std::memcpy(data.key, middle_key, KeyLen);
       
        data.left_child = node->disk_location;
        data.right_child = right_node->disk_location;
        file->update_node(right_node,right_node_location, sizeof(InternalNodeT));
        file->update_node(node,node->disk_location, sizeof(InternalNodeT));
        insert_up_into(data,node->parent);
    }
    else
    {
        NodeT* new_parent = new InternalNodeT();
        off_t new_parent_location = file->alloc_block();
        new_parent->disk_location = new_parent_location;
        root_node = new_parent;

        tree_root = new_parent_location;

        //file->update_root_pointer();
        InternalNodeT* new_parent_cast = static_cast<InternalNodeT*>(new_parent);
        new_parent_cast->children[1] = right_node->disk_location;
        new_parent_cast->children[0] = node->disk_location;
        new_parent->is_leaf = false;
        new_parent->current_key_count = 1;
        node->parent = new_parent->disk_location;
        right_node->parent = new_parent->disk_location;
       
        std::memcpy(new_parent->keys[0], middle_key, KeyLen);
        file->update_node(new_parent, new_parent_location, sizeof(InternalNodeT));
        file->update_node(node, node->disk_location, sizeof(InternalNodeT));
        file->update_node(right_node, right_node->disk_location, sizeof(InternalNodeT));
        
        delete new_parent;
    }

    delete right_node;
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::insert_key_into_node(Insert_Up_Data data, NodeT* node)
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
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
void BtreePlus<NodeT, LeafNodeT, InternalNodeT>::delete_index_in_node(int index, NodeT* node, int child_index)
{
    for (int i = index; i < node->current_key_count-1; i++) {
        std::memcpy(node->keys[i], node->keys[i+1], KeyLen);
    }
    node->current_key_count--;
    if(node->is_leaf)
    {
    }
    else
    {
        InternalNodeT* node_cast = static_cast<InternalNodeT*>(node);
        //adjust children
        int old_count = node_cast->current_key_count;
        //if(is_merge) i++;
        for (int i = child_index+1; i < old_count + 1; i++)
        {
            node_cast->children[i] = node_cast->children[i+1];
           
        }
    }
}
template<typename NodeT, typename LeafNodeT, typename InternalNodeT>
LeafNodeT* BtreePlus<NodeT, LeafNodeT, InternalNodeT>::find_leftmost_leaf() {
    
    NodeT* curr = file->load_node<NodeT>(this->tree_root);
    while (!curr->is_leaf) {
        InternalNodeT* in = static_cast<InternalNodeT*>(curr);
        curr = file->load_node<NodeT>(in->children[0]); // go all the way lef
    }
    return static_cast<LeafNodeT*>(curr);
}

// Explicit instantiations
template class BtreePlus<Node32, LeafNode32, InternalNode32>;
template class BtreePlus<Node16, LeafNode16, InternalNode16>;
template class BtreePlus<Node8, LeafNode8, InternalNode8>;
template class BtreePlus<Node4, LeafNode4, InternalNode4>;