#pragma once
#include "../config.h"
#include "btree.hpp"
#include "../storage/record.hpp"
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

    Table& get_table(const std::string& tableName);

    void update_index_location(Table &table, int column_index, off_t new_index_location);
    int insert(const std::string& tableName, const StringVec& args);
    int select(const std::string& tableName, const StringVec& args);
};