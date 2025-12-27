#pragma once
#include "../config.h"
#include "btree.hpp"
#include "../core/database.hpp"


class TreeCursor
{

public:
    off_t tree_root = 0;
    Database *db = nullptr;
    std::string key;

    virtual bool next() = 0;
    
    virtual ~TreeCursor() = default;

    virtual const std::string get_key() const = 0;

    virtual off_t get_value() const = 0;

    virtual std::string get_key() = 0;

    virtual bool set(const std::string &key) = 0; // set to key

    virtual bool key_equals(const std::string& literal) = 0;
};

template <typename TreeType>
class BPlusTreeCursor : public TreeCursor
{
    

public:
    using NodeType = typename TreeType::NodeType;
    using LeafNodeType = typename TreeType::LeafNodeType;
    using InternalNodeType = typename TreeType::InternalNodeType;

    TreeType* tree = nullptr;

    off_t value;


    LocationData<LeafNodeType> location;


    bool started = false;

    off_t max_keys;


    off_t _readCount = 0;

    BPlusTreeCursor() = default;
    BPlusTreeCursor(TreeType* tree);

    off_t get_value() const override;
    const std::string get_key() const override;
    std::string get_key() override;

    bool next() override;                   // move to next key
    bool set(const std::string &key) override; // set to key

    bool key_equals(const std::string& literal) override;
};
