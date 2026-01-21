#pragma once

#include "../config.h"
#include <vector>



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
    std::vector<size_t> column_lengths;

    std::vector<std::string> to_tokens(const Table& table) const;

    void update_column(int column_index, std::string &value, const Table& table);



    std::string get_token(int index, const Table& table) const;

    Record() = default;

    Record(const StringVec &tokens, const Table &table);

    Record(const std::byte* read_from, const Table &table);

};
