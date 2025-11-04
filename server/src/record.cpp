#include "config.h"
#include "record.hpp"
#include "query.hpp"
#include "file.hpp"
#include "validator.hpp"


std::string strip_quotes(const std::string &input)
{

    if(input[0] != 34 || input[input.size()-1] != 34) return input;

    std::string output = std::string(input.begin()+1, input.end()-1);



    return output;
}

Record::Record(const StringVec &tokens,const Table &table, int offset)
{
    for(long unsigned int i = offset; i < tokens.size(); i++)
    {
        switch (table.columns[i-offset].type)
        {
            case Type::INTEGER:
            {
                if(!validate_INTEGER_token(tokens[i])){
                    length = -1;    //mark to throw error
                    break;
                }
                
                int32_t number = std::stoi(tokens[i]);

                int8_t casted_int[4];

                memcpy(casted_int, &number, 4);

                
                for (int j = 0; j < 4; j++)
                {
                    str += static_cast<char>(casted_int[j]);
                }

                
                
                break;
            }
            case Type::STRING:
            {

                str += strip_quotes(tokens[i]);
                break;
            }
            case Type::UNKNOWN:
            {

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


Record::Record(const std::byte* read_from, Table& table)
{
    char* char_cast = (char*)(read_from);
    int i = 0;

    char int_buffer[4];
    int buffer_index = 0;

    Type current_type = table.columns[i].type; 

    while(*char_cast != ';')
    {
        if(current_type == Type::STRING)
        {
            str += *char_cast;
        }
        else if (current_type == Type::INTEGER)
        {
            int_buffer[buffer_index] = *char_cast;
            buffer_index++;


        }

        if(*char_cast == ' ')
        {
            if(buffer_index != 0)
            {
                int test;

                memcpy(&test, int_buffer, 4);

                buffer_index = 0;

                str.append(std::to_string(test));

                str += ' ';
            }
            i++;
            current_type = table.columns[i].type;
        }

        char_cast++;
    }

    str += ';';
    length = str.length();
}