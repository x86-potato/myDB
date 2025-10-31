#pragma once

#include <string>


class Query; 
class Table;  


class Record
{
public:
    //store in a string
    //handle insertion
    //handle extration 
    //insertion return a const char *
    //extractoon returns, string?

    std::string str;
    int length = 0;


    Record(Query &query, Table &table);

    Record(const std::byte* read_from, Table &table);
    
};