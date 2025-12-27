#include "cursor.hpp"

template <typename TreeType>
BPlusTreeCursor<TreeType>::BPlusTreeCursor(TreeType* tree)
{
    this->tree = tree;

}



template <typename TreeType>
off_t BPlusTreeCursor<TreeType>::get_value() const
{
    return value;
}


template <typename TreeType>
bool BPlusTreeCursor<TreeType>::next()
{
    if(!started)
    {
        started = true;
        return this->set(key);
    }
    //go to next key in leaf //TODO:: later let it follow multiple leaves


    if(location.key_index + 1  >= location.leaf.current_key_count)
    {
        return false;
    } 


    
    location.key_index++;
    value = location.leaf.values[location.key_index];
    return true;
}

template <typename TreeType>
bool BPlusTreeCursor<TreeType>::set(const std::optional<std::string>& key)
{
    
    tree->root_node = db->file->load_node<typename TreeType::NodeType>(tree_root);

    if(key.has_value())
    {
        this->key = *key;
        location = tree->locate(*key);
    }
    else
    {
        location = tree->locate_start();
    }



    if(location.key_index == -1) 
    {
        std::cout << "key: " << key.value() << " not found in cursor set\n";
        return false;
    }

    value = location.leaf.values[location.key_index];

    return true;
}


template <typename TreeType>
const std::string BPlusTreeCursor<TreeType>::get_key() const
{
    if(location.key_index < 0) return std::string();
    return std::string(location.leaf.keys[location.key_index], TreeType::KeyLen);
}

template <typename TreeType>
std::string BPlusTreeCursor<TreeType>::get_key()
{
    if(location.key_index < 0) return std::string();
    return std::string(location.leaf.keys[location.key_index], TreeType::KeyLen);
}



template <typename TreeType>
bool BPlusTreeCursor<TreeType>::key_equals(const std::string& literal)
{
    unsigned char lit[TreeType::KeyLen];
    memset(lit, 0, TreeType::KeyLen);
    memcpy(lit, literal.data(), std::min(literal.size(), (size_t)TreeType::KeyLen));

    return memcmp(get_key().data(), lit, TreeType::KeyLen) == 0;
}



template class BPlusTreeCursor<MyBtree32>;
template class BPlusTreeCursor<MyBtree16>;
template class BPlusTreeCursor<MyBtree8>;
template class BPlusTreeCursor<MyBtree4>;