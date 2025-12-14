#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <iostream>


enum class TokenType
{
    //CREATE tokens
    KW_CREATE,
    KW_TABLE,
    KW_INDEX,
    KW_ON,
    //types
    CHAR_32,
    CHAR_16,
    CHAR_8,
    INT,
    STRING,


    //FIND tokens
    KW_FIND,
    KW_WHERE,
    
    AND,
    OR,
    LESS_THAN,
    GREATER_THAN,
    EQUALS,
    EQUAL_OR_LESS_THAN,
    EQUAL_OR_GREATER_THAN,




    //INSERT tokens
    KW_INSERT,
    KW_INTO,


    //arg tokens
    LITERAL, 
    IDENTIFIER,

    //delimiters
    OPENING_PARENTHESIS,
    CLOSING_PARENTHESIS,
    QUOTE,
    SEMICOLON,
    COMMA,
    SET
};

struct Token
{
    std::string name;
    TokenType type;
};








#include <string>

inline const char* TokenTypeToString(TokenType t) {
    switch (t) {
        case TokenType::KW_CREATE: return "KW_CREATE";
        case TokenType::KW_TABLE: return "KW_TABLE";
        case TokenType::KW_INDEX: return "KW_INDEX";
        case TokenType::KW_ON: return "KW_ON";

        case TokenType::CHAR_32: return "CHAR_32";
        case TokenType::CHAR_16: return "CHAR_16";
        case TokenType::CHAR_8: return "CHAR_8";
        case TokenType::INT: return "INT";
        case TokenType::STRING: return "STRING";

        case TokenType::KW_FIND: return "KW_FIND";
        case TokenType::KW_WHERE: return "KW_WHERE";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::LESS_THAN: return "LESS_THAN";
        case TokenType::GREATER_THAN: return "GREATER_THAN";
        case TokenType::EQUALS: return "EQUALS";
        case TokenType::EQUAL_OR_LESS_THAN: return "EQUAL_OR_LESS_THAN";
        case TokenType::EQUAL_OR_GREATER_THAN: return "EQUAL_OR_GREATER_THAN";
        case TokenType::KW_INSERT: return "KW_INSERT";
        case TokenType::KW_INTO: return "KW_INTO";
        case TokenType::LITERAL: return "LITERAL";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::OPENING_PARENTHESIS: return "OPENING_PARENTHASIS";
        case TokenType::CLOSING_PARENTHESIS: return "CLOSING_PARENTHASIS";
        case TokenType::QUOTE: return "QUOTE";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COMMA:      return "COMMA";
        case TokenType::SET: return "SET";
    }
    return "UNKNOWN";
}

inline TokenType StringToTokenType(const std::string& text) {
    static const std::unordered_map<std::string, TokenType> map = {
        {"create", TokenType::KW_CREATE},
        {"table",  TokenType::KW_TABLE},
        {"index",  TokenType::KW_INDEX},
        {"on",     TokenType::KW_ON},
        {"find",   TokenType::KW_FIND},
        {"where",  TokenType::KW_WHERE},
        {"insert", TokenType::KW_INSERT},
        {"into",   TokenType::KW_INTO},
        {"and",    TokenType::AND},
        {"or",     TokenType::OR},

        {"char32", TokenType::CHAR_32},
        {"char16", TokenType::CHAR_16},
        {"char8",  TokenType::CHAR_8},
        {"int",    TokenType::INT},
        {"string", TokenType::STRING},

        {"(", TokenType::OPENING_PARENTHESIS},
        {")", TokenType::CLOSING_PARENTHESIS},
        {"\"", TokenType::QUOTE},
        {";", TokenType::SEMICOLON},
        {",", TokenType::COMMA},

        {"=",  TokenType::SET},
        {"<",  TokenType::LESS_THAN},
        {">",  TokenType::GREATER_THAN},
        {"==", TokenType::EQUALS},
        {"<=", TokenType::EQUAL_OR_LESS_THAN},
        {">=", TokenType::EQUAL_OR_GREATER_THAN}

        
    };

    if (auto it = map.find(text); it != map.end())
        return it->second;

    return TokenType::IDENTIFIER;
}






