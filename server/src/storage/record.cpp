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



Record::Record(const StringVec& tokens, const Table& table)
{
    str.clear();

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        switch (table.columns[i].type)
        {
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

            // case Type::TEXT:
            // {
            //     // 8-byte heap pointer (placeholder)
            //     uint64_t ptr = 0;
            //     str.append(reinterpret_cast<const char*>(&ptr), sizeof(ptr));
            //     break;
            // }
        }
    }

    length = str.size();
}




Record::Record(const std::byte* read_from, const Table& table)
{
    const char* p = reinterpret_cast<const char*>(read_from);
    str.clear();

    for (size_t i = 0; i < table.columns.size(); ++i)
    {
        switch (table.columns[i].type)
        {
            case Type::INTEGER:
            {
                int32_t v;
                memcpy(&v, p, sizeof(v));
                p += sizeof(v);

                str += std::to_string(v);
                break;
            }

            case Type::CHAR8:
            case Type::CHAR16:
            case Type::CHAR32:
            {
                uint8_t len = static_cast<uint8_t>(*p++);
                str.append(p, p + len);
                p += len;
                break;
            }

            // case Type::TEXT:
            // {
            //     uint64_t ptr;
            //     memcpy(&ptr, p, sizeof(ptr));
            //     p += sizeof(ptr);

            //     str += "<TEXT>";
            //     break;
            // }
        }

        if (i + 1 < table.columns.size())
            str += ' ';
    }

    length = str.size();
}
