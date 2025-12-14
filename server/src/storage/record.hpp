#pragma once

#include <string>


class Table;  
std::string strip_quotes(const std::string &input);

class Record
{
public:
    //store in a string
    //handle insertion
    //handle extration 
    //insertion return a const char *
    //extraction returns, string?

    std::string str;
    int length = 0;

    std::string get_token(int index);


    Record(const StringVec &tokens, const Table &table, int offset);

    Record(const std::byte* read_from, Table &table);
    
};