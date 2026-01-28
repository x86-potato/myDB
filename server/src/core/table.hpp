#pragma once
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "../config.h"
#include "../storage/record.hpp"

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
    std::string type_to_string(const Type &check);
    int type_len(const Type &check);
}



struct Column {
    std::string name;
    Type type;
    uint32_t row_count = 0;
    off_t indexLocation = -1; //0 if not created yet, -1 if not indexable
    Column();
    Column(std::string name, Type type);
};

struct Table {
    std::string name;
    std::vector<Column> columns;

    Table();
    Table(std::byte* data, int len);

    int primaryLen() const;

    void table_print();

    const Column& get_column(int column_index) const;
    const Column& get_column(const std::string& column_name) const;

    int get_column_index(const std::string& column_name) const;


    bool indexed_on_column(int column_index);
    bool indexed_on_column(const std::string& column_name) const;
};

std::vector<std::byte> cast_to_bytes(Table *table);
