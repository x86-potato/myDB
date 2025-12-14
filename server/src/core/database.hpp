#pragma once
#include "../config.h"
#include "btree.hpp"
#include "../storage/record.hpp"

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

    int insert(const std::string& tableName, const StringVec& args);
};