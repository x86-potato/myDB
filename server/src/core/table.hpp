#pragma once
#include <iostream>
#include <string>
#include <vector>

#include "../config.h"

//TYPE defines the type of key that is per column,
//STRING is NOT indexable, must be 
//all indexable columns must be static sized

enum class Type 
{
    CHAR32, 
    CHAR16,
    CHAR8,
    INTEGER,
    TEXT,
    BOOL

};

namespace TypeUtil {
    Type string_to_type(std::string &check);
    std::string type_to_string(Type &check);
    int type_len(Type &check);
}



struct Column {
    std::string name;
    Type type;
    off_t indexLocation = -1; //0 if not created yet, -1 if not indexable
    Column();
    Column(std::string name, Type type);
};

struct Table {
    std::string name;
    std::vector<Column> columns;

    Table();
    Table(std::byte* data, int len);

    int primaryLen();

    void table_print();
};

std::vector<std::byte> cast_to_bytes(Table *table);
