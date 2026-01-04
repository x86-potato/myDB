#include "../config.h"
#include "tokens.hpp"

#include "lexer.hpp"
#include <iostream>


Token Lexer::classify_token(const std::string& text)
{
    if (text.empty())
        return {text, TokenType::IDENTIFIER};

    // boolean literals
    if (text == "true" || text == "false")
        return {text, TokenType::BOOL_LITERAL};

    // numeric literal
    bool all_digits = true;
    for (char c : text)
        if (!std::isdigit(c)) { all_digits = false; break; }

    if (all_digits)
        return {text, TokenType::LITERAL};

    // keywords / operators table
    TokenType t = StringToTokenType(text);

    return {text, t};
}





void Lexer::tokenize(const std::string& input_line, std::vector<Token> &TokenList)
{
    const char* iterator = input_line.data();
    std::string temp;

    while (*iterator != '\0')
    {
        // Skip whitespace
        if (isspace(*iterator))
        {
            if (!temp.empty())
            {
                TokenList.push_back(classify_token(temp));
                temp.clear();
            }
            iterator++;
            continue;
        }

        // Check for operators with lookahead (==, <=, >=)
        if (*iterator == '=' || *iterator == '<' || *iterator == '>' || *iterator == '!')
        {
            if (!temp.empty())
            {
                TokenList.push_back(classify_token(temp));
                temp.clear();
            }

            char next = *(iterator + 1);
            if ((next == '=') && (*iterator == '<' || *iterator == '>' || *iterator == '='))
            {
                std::string op = { *iterator, next };
                TokenList.push_back(classify_token(op));
                iterator += 2;
            }
            else
            {
                TokenList.push_back(classify_token(std::string(1, *iterator)));
                iterator++;
            }
            continue;
        }

        // Single-character delimiters: ( ) , ; [ ] *
        if (*iterator == '(' || *iterator == ')' ||
            *iterator == ',' || *iterator == ';' ||
            *iterator == '[' || *iterator == ']' ||
            *iterator == '*' || *iterator == '.')   
        {
            if (!temp.empty())
            {
                TokenList.push_back(classify_token(temp));
                temp.clear();
            }

            TokenList.push_back(classify_token(std::string(1, *iterator)));
            iterator++;
            continue;
        }

        // Handle string literals (with quotes)
        if (*iterator == '"')
        {
            if (!temp.empty())
            {
                TokenList.push_back(classify_token(temp));
                temp.clear();
            }

            std::string str_content;
            str_content += '"';  // opening quote
            iterator++;

            while (*iterator != '\0' && *iterator != '"')
            {
                str_content += *iterator;
                iterator++;
            }

            if (*iterator == '"')
            {
                str_content += '"';  // closing quote
                iterator++;
            }

            TokenList.push_back({str_content, TokenType::LITERAL});
            continue;
        }

        // Accumulate regular characters (identifiers, keywords, numbers)
        temp += *iterator;
        iterator++;
    }

    // Don't forget the last token
    if (!temp.empty())
    {
        TokenList.push_back(classify_token(temp));
    }

    // Debug output
    // for (const auto& token : TokenList)
    // {
    //     //std::cout << token.name << " " << TokenTypeToString(token.type) << std::endl;
    // }
}