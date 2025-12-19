#pragma once
#include "../config.h"
#include "btree.hpp"



template <typename TreeType>
class BPlusTreeCursor
{
    

public:
    using NodeType = typename TreeType::NodeType;
    using LeafNodeType = typename TreeType::LeafNodeType;
    using InternalNodeType = typename TreeType::InternalNodeType;

    TreeType* tree = nullptr;

    LocationData<LeafNodeType> location;

    bool active = false;

    off_t max_keys;

    off_t _readCount = 0;

    BPlusTreeCursor() = default;
    explicit BPlusTreeCursor(TreeType* tree, const std::string &key);



    //valid() const;            // true if not past end
    void set(TreeType* tree, const std::string &key); // set to key
    void next();                   // move to next key
    void prev();                   // optional
    off_t curr();        // get current record
};
