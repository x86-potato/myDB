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
    if (!started && set_externally) {
        // If set externally but not started, do not advance
        started = true;
        return true;
    }
    if (!started) {
        // Not positioned yet: position once using existing key (or start)
        started = true;

        if (!set(key)) return false;

        if (skip_equals) {
            if (this->key.has_value() && key_equals(this->key.value())) {
                if (location.key_index + 1 >= location.leaf->current_key_count) {
                    return false;
                }
                location.key_index++;
                value = location.leaf->values[location.key_index];
                memcpy(current_key.bytes.data(),
                       location.leaf->keys[location.key_index],
                       TreeType::KeyLen);
            }
        }
        return true;
    }

    //if last leaf
    if (!delete_on_match && location.leaf->next_leaf == 0 && location.key_index + 1 >= location.leaf->current_key_count)
        return false;

    //if at end of current leaf
    if (location.key_index + 1 >= location.leaf->current_key_count) {
        // load next leaf node as pointer
        typename TreeType::NodeType* temp_ptr =
            tree->file->template load_node<typename TreeType::NodeType>(location.leaf->next_leaf);

        // copy the leaf node contents into location.leaf
        location.leaf = static_cast<typename TreeType::LeafNodeType*>(temp_ptr);

        location.key_index = -1;
    }
    if(delete_on_match)
    {
        if(tree_root != tree->tree_root)
        {
            //std::cout << "ROOT CHANGED IN CURSOR FROM: " << tree_root << " TO: " << tree->tree_root << "\n";
            db->update_root_pointer(*table, tree_root, tree->tree_root);
            tree_root = tree->tree_root;
        }
        if(!set(key))
        {
            return false;
        }
        //std::cout << "value at index: " << get_value()<< "\n";
        //if(get_value() == 0)
        //{
        //    std::cout << "ERROR VALUE IS 0\n";
        //}
    }
    else
    {
        location.key_index++;
    }
    value = location.leaf->values[location.key_index];
    current_key.bytes.resize(TreeType::KeyLen);
    memcpy(current_key.bytes.data(),
           location.leaf->keys[location.key_index],
           TreeType::KeyLen);

    return true;
}

template <typename TreeType>
bool BPlusTreeCursor<TreeType>::set(const std::optional<Key>& key)
{
    tree->root_node = db->file->load_node<typename TreeType::NodeType>(tree_root);
    tree->table = table;

    if (key.has_value()) {
        this->key = key;
        normalize_key(this->key.value(), TreeType::KeyLen);

        std::string lookup = std::string(reinterpret_cast<const char*>(this->key->bytes.data()), this->key->bytes.size());
        location = tree->locate(lookup);
    } else {
        location = tree->locate_start();
    }

    if (location.key_index < 0) return false;

    value = location.leaf->values[location.key_index];

    current_key.bytes.resize(TreeType::KeyLen);
    memcpy(current_key.bytes.data(),
           location.leaf->keys[location.key_index],
           TreeType::KeyLen);

    return true;
}



template <typename TreeType>
const Key& BPlusTreeCursor<TreeType>::get_key() const
{
    return current_key;
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