#pragma once
#include "../config.h"
#include "../relational/key.hpp"
#include "btree.hpp"
#include "../core/database.hpp"
#include <optional>



class TreeCursor {
public:
    off_t tree_root = 0;
    Database* db = nullptr;
    std::optional<Key> key;
    Table* table = nullptr;
    bool skip_equals = false;
    bool delete_on_match = false;

    bool set_externally = false;

    virtual bool next() = 0;
    virtual ~TreeCursor() = default;

    virtual const Key& get_key() const = 0;
    virtual off_t get_value() const = 0;
    virtual bool set(const std::optional<Key>& key) = 0;
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


    BPlusTreeCursor() = default;
    BPlusTreeCursor(TreeType* tree);

    off_t get_value() const override;
    const Key& get_key() const override;

    bool next() override;
    bool set(const std::optional<Key>& key) override;
    bool key_equals(const Key& check) override;

private:
    Key current_key;
};
