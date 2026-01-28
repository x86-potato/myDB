#pragma once
#include "../config.h"
#include "btree.hpp"
#include "../query/ast.hpp"
#include "../storage/record.hpp"
#include "../query/plan/planner.hpp"
#include "../query/validator.hpp"
#include "../query/plan/builder.hpp"
#include <arpa/inet.h>
#include <variant>

class File;


class Database
{

public:
    File *file;

    std::unordered_map<std::string,Table> tableMap;

    // define index trees
    MyBtree32 index_tree32;
    MyBtree16 index_tree16;
    MyBtree8 index_tree8;
    MyBtree4 index_tree4;

    Database ();


    void flush();

    const Table& get_table(const std::string& tableName) const;
    Table& get_table(const std::string& tableName);

    void update_index_location(Table &table, int column_index, off_t new_index_location);
    void update_root_pointer(Table &table, off_t old_root, off_t new_root);
    int insert(const std::string& tableName, const StringVec& args);
    int erase(const std::string& tableName, Plan &plan);
};
