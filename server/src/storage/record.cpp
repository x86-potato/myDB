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

std::string Record::get_token(int index)
{
    std::string token;
    int curr_index = 0;
    char* parser = str.data();

    while(curr_index != index)
    {
        if(parser[0] == ' ')
        {
            curr_index++;
        }
        parser++;
    }

    while (parser[0] != ' ' && parser[0] != ';')
    {
        token += parser[0];
        parser++;
    }

    return token;
}

Record::Record(const StringVec &tokens,const Table &table, int offset)
{
    for(long unsigned int i = offset; i < tokens.size(); i++)
    {
        switch (table.columns[i-offset].type)
        {
            case Type::INTEGER:
            {
                //if(!validate_INTEGER_token(tokens[i])){
                //    length = -1;    //mark to throw error
                //    break;
                //}
                
                int32_t number = std::stoi(tokens[i]);

                int8_t casted_int[4];

                memcpy(casted_int, &number, 4);

                
                for (int j = 0; j < 4; j++)
                {
                    str += static_cast<char>(casted_int[j]);
                }

                
                
                break;
            }
            case Type::CHAR32:
            {
                str += strip_quotes(tokens[i]);
                break;
            }
            case Type::CHAR16:
            {
                str += strip_quotes(tokens[i]);
                break;
            }
            case Type::CHAR8:
            {
                str += strip_quotes(tokens[i]);
                break;
            }

        }
        if(length == -1)
        {
            break;
        }
        str += " ";
    } 

    if(length != -1)
    {
        str[str.length()-1] = ';';
        length = str.length();
    } 

}


Record::Record(const std::byte* read_from, Table& table) {
    const char* p = reinterpret_cast<const char*>(read_from);
    str.clear();

    for (size_t i = 0; i < table.columns.size(); ++i) {
        Type t = table.columns[i].type;

        if (t == Type::INTEGER) {
            int val;
            memcpy(&val, p, sizeof(int));
           // 24423112
            str += std::to_string(val);
            p += sizeof(int);
        } else {
            // Read until space or semicolon
            while (*p != ' ' && *p != ';') {
                str += *p;
                p++;
            }
        }

        if (*p == ' ') {
            str += ' ';
            p++;
        } else if (*p == ';') {
            str += ';';
            break;
        }
    }

    length = str.length();
}
