#include "cursor.hpp"

template <typename TreeType>
BPlusTreeCursor<TreeType>::BPlusTreeCursor(TreeType* type, const std::string &key)
{
    


    
}



template <typename TreeType>
void BPlusTreeCursor<TreeType>::next()
{

}

template <typename TreeType>
void BPlusTreeCursor<TreeType>::set(TreeType* type, const std::string &key)
{
    
    //set cursor to leaf with data
    max_keys = TreeType::MaxKeys;

    tree = type;

    location = tree->locate(key);
    if(location.key_index == -1) 
    {
        std::cout << "not found";
        return;
    }

    active = true;


    //loop while there are still keys in this leaf, and they equal the key
    while(location.key_index < location.leaf.current_key_count && 
          std::memcmp(location.leaf.keys[location.key_index], key.c_str(), TreeType::KeyLen) == 0)
    {
        std::cout << "Found record at location: " << location.leaf.values[location.key_index] << "\n";
        location.key_index++;
    }
}

template <typename TreeType>
off_t BPlusTreeCursor<TreeType>::curr()
{
    if(location.key_index < location.leaf.current_key_count && active)
    {
        return location.leaf.values[location.key_index];
    }
    return -1;
}


template class BPlusTreeCursor<MyBtree32>;
template class BPlusTreeCursor<MyBtree16>;
template class BPlusTreeCursor<MyBtree8>;
template class BPlusTreeCursor<MyBtree4>;