#include "record.hpp"
#include "query.hpp"
#include "file.hpp"


std::string strip_quotes(const std::string &input)
{
     
}

Record::Record(Query &query, Table &table)
{
    if(str.length() != 0) str.clear();
    for(int i = 0; i < query.args.size(); i++)
    {
        int entry_len = 0;
        switch (table.columns[i].type)
        {
            case Type::INTEGER:
            {
                entry_len = 4;

                int32_t number = std::stoi(query.args[i].rhs);

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
                
                entry_len = query.args[i].rhs.length();
                str += query.args[i].rhs;
                break;
            }
        }

        str += " ";
    } 
     
    str[str.length()-1] = ';';
    length = str.length();

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