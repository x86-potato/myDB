#include "../config.h"
#include "record.hpp"
#include "../storage/file.hpp"
#include "../query/validator.hpp"


std::string strip_quotes(const std::string &input)
{

    if(input[0] != 34 || input[input.size()-1] != 34) return input;

    std::string output = std::string(input.begin()+1, input.end()-1);



    return output;
}

void Record::update_column(int column_index, std::string &value, const Table& table)
{
    size_t offset = 0;
    for (int i = 0; i < column_index; ++i)
        offset += column_lengths[i];

    auto type = table.columns[column_index].type;

    switch (type)
    {
        case Type::BOOL:
        case Type::INTEGER:
        {
            memcpy(str.data() + offset, value.data(), value.size());
            return;
        }

        case Type::CHAR8:
        case Type::CHAR16:
        case Type::CHAR32:
        case Type::TEXT:
        {
            uint8_t new_len = static_cast<uint8_t>(value.size());
            // Note: offset points to the length byte, not the data
            uint8_t old_len = static_cast<uint8_t>(str[offset]);

            size_t old_total = column_lengths[column_index];
            size_t new_total = 1 + new_len;

            int delta = static_cast<int>(new_total) - static_cast<int>(old_total);

            size_t tail_src = offset + old_total;
            size_t tail_size = str.size() - tail_src;

            if (delta > 0)
            {
                // EXPANDING: Resize first to allocate memory, then shift tail right
                str.resize(str.size() + delta);
                memmove(
                    str.data() + offset + new_total, // Dest
                    str.data() + tail_src,           // Src (old position)
                    tail_size
                );
            }
            else if (delta < 0)
            {
                // SHRINKING: Shift tail left FIRST to save data, then trim the end
                memmove(
                    str.data() + offset + new_total, // Dest (new position)
                    str.data() + tail_src,           // Src
                    tail_size
                );
                str.resize(str.size() + delta);
            }

            // Write new length byte
            str[offset] = static_cast<char>(new_len);

            // Write payload
            memcpy(str.data() + offset + 1, value.data(), new_len);

            // Update cached column size
            column_lengths[column_index] = new_total;

            // Update record length
            length = str.size();

            return;
        }
    }
}
std::vector<std::string> Record::to_tokens(const Table& table) const {
    std::vector<std::string> tokens;
    size_t offset = 0;

    for (size_t i = 0; i < table.columns.size(); ++i) {
        std::string token;

        switch (table.columns[i].type) {
            case Type::BOOL:
                token = (str[offset] == '1') ? "true" : "false";
                break;

            case Type::INTEGER:
            {
                int32_t v;
                memcpy(&v, str.data() + offset, sizeof(v));
                token = std::to_string(v);
                break;
            }

            case Type::CHAR8:
            case Type::CHAR16:
            case Type::CHAR32:
            case Type::TEXT:
            {
                uint8_t len = static_cast<uint8_t>(str[offset]);
                token = str.substr(offset + 1, len);
                break;
            }
        }

        tokens.push_back(token);
        offset += column_lengths[i];
    }

    return tokens;
}



std::string Record::get_token(int index, const Table& table) const
{
    if (index < 0 || index >= static_cast<int>(table.columns.size()))
        throw std::out_of_range("Record::get_token: index out of range");

    size_t offset = 0;
    for (int i = 0; i < index; ++i)
    {
        offset += column_lengths[i];
    }

    // Handle like to_tokens does
    switch (table.columns[index].type)
    {
        case Type::BOOL:
            return (str[offset] == '1') ? "true" : "false";

        case Type::INTEGER:
        {
            int32_t v;
            memcpy(&v, str.data() + offset, sizeof(v));
            return std::to_string(v);
        }

        case Type::CHAR8:
        case Type::CHAR16:
        case Type::CHAR32:
        case Type::TEXT:
        {
            uint8_t len = static_cast<uint8_t>(str[offset]);
            return str.substr(offset + 1, len);  // Skip length byte
        }
    }

    return "";
}
Record::Record(const StringVec& tokens, const Table& table)
{
    str.clear();
    column_lengths.clear();  // Add this

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        size_t col_len = 0;  // Track column length
        switch (table.columns[i].type)
        {
            case Type::BOOL:
            {
                bool v = (tokens[i] == "true");
                str.push_back(v ? '1' : '0');
                col_len = 1;
                break;
            }
            case Type::INTEGER:
            {
                int32_t v = std::stoi(tokens[i]);
                str.append(reinterpret_cast<const char*>(&v), sizeof(v));
                col_len = sizeof(v);
                break;
            }

            case Type::CHAR8:
            case Type::CHAR16:
            case Type::CHAR32:
            {
                std::string value = strip_quotes(tokens[i]);

                uint8_t len = static_cast<uint8_t>(value.size());
                str.push_back(static_cast<char>(len));
                str.append(value.data(), len);
                col_len = 1 + len;
                break;
            }
            case Type::TEXT:
            {
                std::string value = strip_quotes(tokens[i]);

                uint8_t len = static_cast<uint8_t>(value.size());
                str.push_back(static_cast<char>(len));
                str.append(value.data(), len);
                col_len = 1 + len;
                break;
            }
        }
        column_lengths.push_back(col_len);  // Add this
    }

    length = str.size();
}




Record::Record(const std::byte* read_from, const Table& table)
{
    const char* p = reinterpret_cast<const char*>(read_from);
    str.clear();
    column_lengths.clear();

    for (size_t i = 0; i < table.columns.size(); ++i)
    {
        size_t col_len = 0;
        switch (table.columns[i].type)
        {
            case Type::BOOL:
                str += (*p++ == '1') ? "true" : "false";
                col_len = 1;
                break;
            case Type::INTEGER:
            {
                int32_t v;
                memcpy(&v, p, sizeof(v));
                p += sizeof(v);
                str.append(reinterpret_cast<const char*>(&v), sizeof(v));
                col_len = sizeof(v);
                break;
            }
            case Type::CHAR8:
            case Type::CHAR16:
            case Type::CHAR32:
            case Type::TEXT:
            {
                uint8_t len = static_cast<uint8_t>(*p++);
                str.push_back(static_cast<char>(len));
                str.append(p, len);
                p += len;
                col_len = 1 + len;
                break;
            }
        }
        column_lengths.push_back(col_len);
    }

    length = str.size();
}
