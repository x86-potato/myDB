#include "../config.h"
#include "table.hpp"
#include <cstring>

namespace TypeUtil {

Type string_to_type(std::string &check)
{
    if (check == "int") {
        return Type::INTEGER;
    }
    else if (check == "char32")
    {
        return Type::CHAR32;
    }
    else if (check == "char16")
    {
        return Type::CHAR16;
    }
    else if( check == "char8")
    {
        return Type::CHAR8;
    }
    else if (check == "text") {
        return Type::TEXT;
    }
    else if (check == "bool") {
        return Type::BOOL;
    }
    return Type::INTEGER;
}

std::string type_to_string(const Type &check)
{
    switch (check) {
        case Type::CHAR32:  return "char32";
        case Type::CHAR16:  return "char16";
        case Type::CHAR8:   return "char8";
        case Type::INTEGER: return "int";
        case Type::TEXT:  return "text";
        case Type::BOOL: return "bool";
        default:            return "unknown";
    }
}

int type_len(const Type &check)
{
    switch (check) {
        case Type::INTEGER: return 4;
        case Type::CHAR32:  return 32;
        case Type::CHAR16:  return 16;
        case Type::CHAR8:   return 8;
        case Type::TEXT:  return 32;
        case Type::BOOL: return 1;
        default:            return 0;
    }

    return 0;
}

} // namespace TypeUtil


// ----------------------- Column -----------------------
Column::Column() {}

Column::Column(std::string name, Type type)
    : name(name), type(type)
{
}

// ----------------------- Table -----------------------
Table::Table()
{
}

Table::Table(std::byte* data, int len)
{
    const std::byte delimiter = static_cast<std::byte>(0x1F);
    const std::byte table_end = static_cast<std::byte>(0x1E);

    if (data == nullptr || len <= 0) {
        return;
    }

    int pos = 0;

    auto read_text_until_delimiter = [&](std::string& out) -> bool {
        out.clear();
        while (pos < len && data[pos] != delimiter && data[pos] != table_end) {
            out += static_cast<char>(data[pos]);
            ++pos;
        }

        if (pos >= len || data[pos] != delimiter) {
            return false;
        }

        ++pos;
        return true;
    };

    if (!read_text_until_delimiter(name)) {
        return;
    }

    if (pos + static_cast<int>(sizeof(current_record_block_location)) > len) {
        return;
    }
    std::memcpy(&current_record_block_location, data + pos, sizeof(current_record_block_location));
    pos += static_cast<int>(sizeof(current_record_block_location));
    if (pos >= len || data[pos] != delimiter) {
        return;
    }
    ++pos;

    if (pos + static_cast<int>(sizeof(row_count)) > len) {
        return;
    }
    std::memcpy(&row_count, data + pos, sizeof(row_count));
    pos += static_cast<int>(sizeof(row_count));
    if (pos >= len || data[pos] != delimiter) {
        return;
    }
    ++pos;

    while (pos < len && data[pos] != table_end) {
        Column col;

        if (!read_text_until_delimiter(col.name)) {
            break;
        }

        if (pos >= len || data[pos] == table_end) {
            break;
        }
        col.type = static_cast<Type>(data[pos]);
        ++pos;

        if (pos >= len || data[pos] != delimiter) {
            break;
        }
        ++pos;

        if (pos + static_cast<int>(sizeof(col.indexLocation)) > len) {
            break;
        }
        std::memcpy(&col.indexLocation, data + pos, sizeof(col.indexLocation));
        pos += static_cast<int>(sizeof(col.indexLocation));

        if (pos < len && data[pos] == delimiter) {
            ++pos;
            columns.push_back(col);
            continue;
        }

        if (pos < len && data[pos] == table_end) {
            columns.push_back(col);
            break;
        }

        break;
    }
}


void Table::table_print()
{
    std::cout << "\ntable name: " << name << std::endl;
    std::cout << "current record block location: " << current_record_block_location << std::endl;
    std::cout << "row count: " << row_count << std::endl;
    for (const auto &i : columns) {
        std::cout << "column: " << i.name << " " << TypeUtil::type_to_string(i.type) << i.indexLocation << std::endl;
    }
}

int Table::primaryLen() const
{
    switch (columns[0].type)
    {
        case (Type::BOOL):
            return 1;
        case (Type::INTEGER):
            return 4;
        case (Type::CHAR32):
            return 32;
        case (Type::CHAR16):
            return 16;
        case (Type::CHAR8):
            return 8;
        case (Type::TEXT):
            return 0;

    }
    return 0;
}



std::vector<std::byte> cast_to_bytes(Table *table)
{
    std::vector<std::byte> output;
    const std::byte delimiter = static_cast<std::byte>(0x1F);
    const std::byte table_end = static_cast<std::byte>(0x1E);

    // push table name
    for (auto &c : table->name)
        output.push_back(static_cast<std::byte>(c));
    output.push_back(delimiter);

    // push current record block location
    std::byte *recordBlockParser = reinterpret_cast<std::byte*>(&table->current_record_block_location);
    for (int i = 0; i < sizeof(table->current_record_block_location); i++)
        output.push_back(recordBlockParser[i]);
    output.push_back(delimiter);

    // push row count
    std::byte *rowCountParser = reinterpret_cast<std::byte*>(&table->row_count);
    for (int i = 0; i < sizeof(table->row_count); i++)
        output.push_back(rowCountParser[i]);
    output.push_back(delimiter);

    // push columns
    for (auto &col : table->columns) {
        // column name
        for (auto &c : col.name)
            output.push_back(static_cast<std::byte>(c));
        output.push_back(delimiter);

        // type as byte
        output.push_back(static_cast<std::byte>(col.type));
        output.push_back(delimiter);

        // indexLocation (8 bytes)
        std::byte *offsetParser = reinterpret_cast<std::byte*>(&col.indexLocation);
        for (int i = 0; i < 8; i++)
            output.push_back(offsetParser[i]);

        output.push_back(delimiter);
    }

    output.push_back(table_end);
    return output;
}

bool Table::indexed_on_column(int column_index) {
    if (column_index < 0 || column_index >= static_cast<int>(columns.size())) {
        return false;
    }
    return columns[column_index].indexLocation != -1;
}




bool Table::indexed_on_column(const std::string& column_name) const {
    for (const auto& col : columns) {
        if (col.name == column_name) {
            return col.indexLocation != -1;
        }
    }
    return false;
}

int Table::get_column_index(const std::string& column_name) const {
    for (int i = 0; i < static_cast<int>(columns.size()); i++) {
        if (columns[i].name == column_name) {
            return i;
        }
    }
    return -1;
}

const Column& Table::get_column(int column_index) const {
    if (column_index < 0 || column_index >= static_cast<int>(columns.size())) {
        throw std::out_of_range("Column index out of range");
    }
    return columns[column_index];
}
const Column& Table::get_column(const std::string& column_name) const {
    for (const auto& col : columns) {
        if (col.name == column_name) {
            return col;
        }
    }
    throw std::invalid_argument("Column name not found: " + column_name);
}
