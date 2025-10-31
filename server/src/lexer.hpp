#pragma once
#include <iostream>

#include <string>
#include <vector>
#include "table.hpp"




struct Lexer_Output
{
    std::string command;
    std::string table;
    std::vector<std::string> arg_strings;
};



class Lexer
{
public:

    Lexer(){}

    

    Lexer_Output lexer(std::string &input)
    { 
        Lexer_Output output;
        tokenize(input, output);

        

        return output;
    }

    void tokenize(std::string &input, Lexer_Output &output)
    {
        std::vector<std::string> tokens;
        //std::cout << input;
        std::string temp;
        temp.reserve(20);
        for (long unsigned int i = 0; i < input.length(); i++)
        {
            if(input[i] != ' ' && input[i] != ','&& input[i] != '(' && input[i] != ')' && input[i] != ';')    
            {
                temp += input[i];
            }
            else
            {
                if(temp.length() > 1)
                {
                    if(tokens.size() > 1)
                    {
                        output.arg_strings.push_back(temp);
                    }
                    else
                    {
                        tokens.push_back(temp);
                    }
                    temp.clear();
                }
                
            }



        }

        output.command  = tokens[0];
        output.table    = tokens[1];

    }
    

};


