#include "cursor.hpp"


inline void normalize_key(Key& k, size_t key_len) {
    if (k.bytes.size() != key_len) {
        k.bytes.resize(key_len, std::byte{0}); // pad with zeros if too short
    }
}



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
bool BPlusTreeCursor<TreeType>::next() {

    location.key_index++; 
    if(location.leaf == nullptr)
    {
        return false;
    }
    if(location.key_index >= location.leaf->current_key_count) 
    {
        if(location.leaf->next_leaf == 0) 
        {
            return false; // end of tree
        } 
        else 
        {
            //load next 
            location.leaf = static_cast<typename TreeType::LeafNodeType*>(
                tree->file->template load_node<typename TreeType::NodeType>(
                location.leaf->next_leaf
                )
            );
            leaves_read++;
            location.key_index = 0;
        }
    }


    //update current key and value
    update_key_and_value(); 

    //commit_progress();
    return true;

}


template <typename TreeType>
void BPlusTreeCursor<TreeType>::skip_read_leaves()
{
    int target_skip = std::max(0, leaves_read -2);
    //int target_skip = 0;
    int skipped = 0;

    //std::cout << "Skipping " << target_skip << " leaves\n";

    while (skipped < target_skip && location.leaf->next_leaf != 0)
    {
        location.leaf = static_cast<typename TreeType::LeafNodeType*>(
            tree->file->template load_node<typename TreeType::NodeType>(
                location.leaf->next_leaf
            )
        );
        location.key_index = 0;
        skipped++;
    }

    //can never skip to the end
    //assert(location.leaf->next_leaf != 0);
    leaves_read -= 2;


}

template <typename TreeType>
void BPlusTreeCursor<TreeType>::commit_progress()
{
    if(leaves_read == 0)
        return;
    leaves_commited += leaves_read - 1;
    leaves_read = std::max(0, leaves_commited);

}

template <typename TreeType>
bool BPlusTreeCursor<TreeType>::set_start()
{
    tree->tree_root = table->columns[column_index].indexLocation;
    this->tree_root = tree->tree_root;

    tree->root_node =
        tree->file->template load_node<typename TreeType::NodeType>(tree->tree_root);



    location = tree->locate_start();
    if (location.key_index == -1 || location.leaf == nullptr || tree->root_node->current_key_count == 0)
    {
        return false;
    }

    update_key_and_value();

    return true;
}


template <typename TreeType>
bool BPlusTreeCursor<TreeType>::set_gte(const Key& key)
{
    //std::cout << "set tree root to " << table->columns[column_index].indexLocation << "\n";
    tree->tree_root = table->columns[column_index].indexLocation;
    this->tree_root = tree->tree_root;
    tree->root_node =
        tree->file->template load_node<typename TreeType::NodeType>(tree->tree_root);

    Key k = key;
    normalize_key(k, TreeType::KeyLen);
    location = tree->locate_gte(std::string(reinterpret_cast<const char*>(k.bytes.data()), TreeType::KeyLen));
    if (location.key_index == -1)
    {
        return false;
    }

    update_key_and_value();
    return true;
}
template <typename TreeType>
bool BPlusTreeCursor<TreeType>::set_gt(const Key &key)
{
    //std::cout << "set tree root to " << table->columns[column_index].indexLocation << "\n";
    tree->tree_root = table->columns[column_index].indexLocation;
    this->tree_root = tree->tree_root;
    tree->root_node =
        tree->file->template load_node<typename TreeType::NodeType>(tree->tree_root);

    Key k = key;
    normalize_key(k, TreeType::KeyLen);
    location = tree->locate_gt(std::string(reinterpret_cast<const char*>(k.bytes.data()), k.bytes.size()));

    if(location.key_index == -1)
    {
        return false;
    }

    update_key_and_value();
    return true;
}



template <typename TreeType>
const Key& BPlusTreeCursor<TreeType>::get_key() const
{
    return current_key;
}
template <typename TreeType>
void BPlusTreeCursor<TreeType>::update_key_and_value()
{
    current_key.bytes.resize(TreeType::KeyLen);
    std::memcpy(current_key.bytes.data(), location.leaf->keys[location.key_index], TreeType::KeyLen);

    value = location.leaf->values[location.key_index];
}



template <typename TreeType>
bool BPlusTreeCursor<TreeType>::key_equals(const Key& check)
{
    if (check.bytes.size() != TreeType::KeyLen)
        return false;

    return memcmp(current_key.bytes.data(),
                  check.bytes.data(),
                  TreeType::KeyLen) == 0;
}




template class BPlusTreeCursor<MyBtree32>;
template class BPlusTreeCursor<MyBtree16>;
template class BPlusTreeCursor<MyBtree8>;
template class BPlusTreeCursor<MyBtree4>;
