#include "../config.h"
#include "tokens.hpp"

#include "lexer.hpp"
#include <iostream>



Token Lexer::classify_token(const std::string& text)
{
    if (text.empty())
        return {text, TokenType::IDENTIFIER};

    // numeric literal
    bool all_digits = true;
    for (char c : text)
        if (!std::isdigit(c)) { all_digits = false; break; }

    if (all_digits)
        return {text, TokenType::LITERAL};

    // string literal classification happens in tokens.hpp table
    TokenType t = StringToTokenType(text);

    // fallback to IDENTIFIER
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
        
        // Check for operators with lookahead
        if (*iterator == '=' || *iterator == '<' || 
            *iterator == '>' || *iterator == '!')
        {
            if (!temp.empty())
            {
                std::string op(1, '\0');
                op[0] = *iterator;
                TokenList.push_back(classify_token(temp));
                
                temp.clear();
            }
            
            // Lookahead for two-character operators
            char next = *(iterator + 1);
            if ((*iterator == '=' || *iterator == '<' || *iterator == '>') && next == '=')
            {
                std::string op(2, '\0');
                op[0] = *iterator;
                op[1] = next;
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
        
        // Check for single-character delimiters
        if (*iterator == '(' || *iterator == ')' || 
            *iterator == ',' || *iterator == ';')
        {
            if (!temp.empty())
            {
                TokenList.push_back(classify_token(temp));
                temp.clear();
            }
            
            TokenList.push_back(classify_token(std::string(iterator, 1)));
            iterator++;
            continue;
        }
        
        // Handle string literals, keep quotes
        if (*iterator == '"')
        {
            if (!temp.empty())
            {
                TokenList.push_back(classify_token(temp));
                temp.clear();
            }

            std::string str_content;
            str_content += '"';  // include opening quote
            iterator++;          

            while (*iterator != '\0' && *iterator != '"')
            {
                str_content += *iterator;
                iterator++;
            }

            if (*iterator == '"') {
                str_content += '"'; // include closing quote
                iterator++;          // move past closing quote
            }
            
            TokenList.push_back({str_content, TokenType::LITERAL});
            continue;
        }

        
        // Regular character - accumulate
        temp += *iterator;
        iterator++;
    }
    
    if (!temp.empty())
    {
        TokenList.push_back(classify_token(temp));
    }

    for (auto token :TokenList)
    {
        //std::cout << token.name << " " << TokenTypeToString(token.type) << std::endl;
    }
        
}
