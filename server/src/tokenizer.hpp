#pragma once
#include <iostream>
#include <string>
#include <vector>


#include "config.h"







inline void tokenize(const std::string &input, StringVec &output)
{
    std::string temp;
    bool in_quotes = false;

    for (char c : input)
    {
        if (c == '"') {
            in_quotes = !in_quotes;
            temp += c;
        }
        else if (!in_quotes && (c == ' ' || c == ',' || c == '(' || c == ')' || c == ';'))
        {
            if (!temp.empty()) {
                output.push_back(temp);
                temp.clear();
            }
        }
        else {
            temp += c;
        }
    }
    
    if (!temp.empty() && temp[0] != ' ' && temp[0] != ';')
        output.push_back(temp);
}
