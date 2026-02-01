#pragma once
#include "../config.h"
#include "../relational/key.hpp"
#include "btree.hpp"
#include "../core/database.hpp"
#include <optional>
#include <algorithm>



class TreeCursor {
public:
    off_t tree_root = 0;
    Database* db = nullptr;
    int column_index = 0; //to do, only works for primairy column
    Table* table = nullptr;

    virtual bool next() = 0;
    virtual ~TreeCursor() = default;

    virtual const Key& get_key() const = 0;
    virtual off_t get_value() const = 0;


    virtual void skip_read_leaves() = 0;
    virtual void commit_progress() = 0;

    virtual bool set_start() = 0;
    virtual bool set_gt(const Key& key) = 0;
    virtual bool set_gte(const Key& key) = 0;

    virtual bool key_equals(const Key& check) = 0;
};


template <typename TreeType>
class BPlusTreeCursor : public TreeCursor
{
public:
    TreeType* tree = nullptr;
    off_t value;
    LocationData<typename TreeType::LeafNodeType> location;
    bool started = false;
    int leaves_read = 0;
    int leaves_commited = 0;


    BPlusTreeCursor() = default;
    BPlusTreeCursor(TreeType* tree);

    off_t get_value() const override;
    const Key& get_key() const override;

    bool next() override;

    void skip_read_leaves() override;
    void commit_progress() override;

    bool set_start() override;
    bool set_gte(const Key& key) override;
    bool set_gt(const Key& key) override;

    bool key_equals(const Key& check) override;
    void update_key_and_value();

private:
    Key current_key;
};
