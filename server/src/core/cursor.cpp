#include "cursor.hpp"

template <typename TreeType>
BPlusTreeCursor<TreeType>::BPlusTreeCursor(TreeType* type, const std::string &key)
{
    this->tree = type;
    
    //set cursor to leaf with data
    max_keys = TreeType::MaxKeys;

    location = tree->locate(key);
    if(!location.leaf) std::cout << "not found";
    else
    std::cout << location.index;

    
}


template <typename TreeType>
void BPlusTreeCursor<TreeType>::next()
{

}




template class BPlusTreeCursor<MyBtree32>;