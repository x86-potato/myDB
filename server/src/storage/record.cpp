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



std::string Record::get_token(int index, const Table& table)
{
    if (index < 0 || index >= static_cast<int>(table.columns.size()))
        throw std::out_of_range("Record::get_token: index out of range");

    size_t offset = 0;
    for (int i = 0; i < index; ++i)
        offset += column_lengths[i];

    return str.substr(offset, column_lengths[index]);
}


Record::Record(const StringVec& tokens, const Table& table)
{
    str.clear();

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        switch (table.columns[i].type)
        {
            case Type::BOOL:
            {
                bool v = (tokens[i] == "true");
                str.push_back(v ? '1' : '0');
                break;
            }
            case Type::INTEGER:
            {
                int32_t v = std::stoi(tokens[i]);
                str.append(reinterpret_cast<const char*>(&v), sizeof(v));
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
                break;
            }
            case Type::TEXT:
            {
                //place holder, later will be replaced by file pointer
                std::string value = strip_quotes(tokens[i]);

                uint8_t len = static_cast<uint8_t>(value.size());
                str.push_back(static_cast<char>(len));
                str.append(value.data(), len);
                break;
            }
        }
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
                str.append(p, len);
                p += len;
                col_len = len;
                break;
            }
        }
        column_lengths.push_back(col_len);
    }

    length = str.size();
}

