#include "../config.h"
#include "table.hpp"

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

std::string type_to_string(Type &check)
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

int type_len(Type &check)
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

    std::string buffer;
    Column new_column;
    enum State { TABLE_NAME, COL_NAME, COL_TYPE, COL_INDEX } state = TABLE_NAME;

    for (int i = 0; i < len; i++) {
        std::byte b = data[i];

        if (b == delimiter || b == table_end) {
            switch (state) {
                case TABLE_NAME:
                    name = buffer;
                    state = COL_NAME;
                    break;
                case COL_NAME:
                    new_column.name = buffer;
                    state = COL_TYPE;
                    break;
                case COL_TYPE:
                    new_column.type = static_cast<Type>(buffer[0]);
                    state = COL_INDEX;
                    break;
                case COL_INDEX:
                    if (!buffer.empty()) {
                        new_column.indexLocation = *reinterpret_cast<off_t*>(&buffer[0]);
                    }
                    columns.push_back(new_column);
                    new_column = Column();
                    state = COL_NAME;
                    break;
            }
            buffer.clear();

            if (b == table_end)
                break;
        } else {
            buffer += static_cast<char>(b);
        }
    }
}


void Table::table_print()
{
    std::cout << "\ntable name: " << name << std::endl;
    for (auto &i : columns) {
        std::cout << "column: " << i.name << " " << TypeUtil::type_to_string(i.type) << i.indexLocation << std::endl;
    }
}

int Table::primaryLen()
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

Column& Table::get_column(int column_index) {
    if (column_index < 0 || column_index >= static_cast<int>(columns.size())) {
        throw std::out_of_range("Column index out of range");
    }
    return columns[column_index];
}
Column& Table::get_column(const std::string& column_name) {
    for (auto& col : columns) {
        if (col.name == column_name) {
            return col;
        }
    }
    throw std::invalid_argument("Column name not found: " + column_name);
}