#include "config.h"
#include "table.hpp"

namespace TypeUtil {

Type string_to_type(std::string &check)
{
    if (check == "int") {
        return Type::INTEGER;
    } else if (check == "string") {
        return Type::STRING;
    }
    return Type::UNKNOWN;
}

std::string type_to_string(Type &check)
{
    switch (check) {
        case Type::INTEGER: return "int";
        case Type::STRING:  return "string";
        default:            return "unknown";
    }
}

int type_len(Type &check)
{
    switch (check) {
        case Type::INTEGER: return 4;
        case Type::STRING:  return 32;
        default:            return -1;
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
Table::Table() : name("none")
{
}

Table::Table(std::byte* data, int len)
{
    std::string buffer;
    bool table_name_picked = false;
    bool name_picked = false;

    Column new_column;

    for (int i = 0; i < len; i++) {
        char input_char = static_cast<char>(data[i]);

        if (input_char == ' ') {
            if (!table_name_picked) {
                name = buffer;
                table_name_picked = true;
            } else if (!name_picked) {
                new_column.name = buffer;
                name_picked = true;
            } else {
                uint8_t type_val = static_cast<uint8_t>(buffer[0]);
                new_column.type = static_cast<Type>(type_val);
                columns.push_back(new_column);

                new_column = Column();
                name_picked = false;
            }
            buffer.clear();
        } else if (input_char == ';') {
            if (name_picked && !buffer.empty()) {
                uint8_t type_val = static_cast<uint8_t>(buffer[0]);
                new_column.type = static_cast<Type>(type_val);
                columns.push_back(new_column);
            }
            break;
        } else {
            buffer += input_char;
        }
    }
}

void Table::table_print()
{
    std::cout << "\ntable name: " << name << std::endl;
    for (auto &i : columns) {
        std::cout << "column: " << i.name << " " << TypeUtil::type_to_string(i.type) << std::endl;
    }
}

int Table::primaryLen()
{
    switch (columns[0].type)
    {
        case (Type::INTEGER):
        {
            return 4;
        }
        case (Type::STRING):
        {
            return 32;
        }
        case (Type::UNKNOWN):
        {
            return 0;
        }
    }
    return 0;
}



// ----------------------- cast_to_bytes -----------------------
std::vector<std::byte> cast_to_bytes(Table *table)
{
    std::vector<std::byte> output;

    // push table name
    for (auto &c : table->name)
        output.push_back(static_cast<std::byte>(c));
    output.push_back(static_cast<std::byte>(' '));

    // push columns
    for (auto &col : table->columns) {
        for (auto &c : col.name)
            output.push_back(static_cast<std::byte>(c));
        output.push_back(static_cast<std::byte>(' '));

        // push type as byte
        output.push_back(static_cast<std::byte>(col.type));
        output.push_back(static_cast<std::byte>(' '));
    }

    output.push_back(static_cast<std::byte>(';'));
    return output;
}
