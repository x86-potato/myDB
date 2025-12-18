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

    //CREATE flags
    BRACKET_OPEN,
    BRACKET_CLOSE,
    FLAG_INDEXED,
    FLAG_PRIMARY,

    

    //types
    CHAR_32,
    CHAR_16,
    CHAR_8,
    INT,
    TEXT,
    BOOL,


    //SELECT tokens
    KW_SELECT,
    KW_WHERE,
    ASTERISK,
    KW_FROM,



    //MODIFY tokens
    KW_MODIFY,
    KW_SET,
    
    AND,
    OR,
    LESS_THAN,
    GREATER_THAN,
    EQUALS,
    EQUAL_OR_LESS_THAN,
    EQUAL_OR_GREATER_THAN,



    //Helper tokens
    KW_SHOW,
    KW_PRAGMA,
    KW_INFO,

    //INSERT tokens
    KW_INSERT,
    KW_INTO,


    //arg tokens
    LITERAL, 
    IDENTIFIER,
    BOOL_LITERAL,

    //delimiters
    OPENING_PARENTHESIS,
    CLOSING_PARENTHESIS,
    QUOTE,
    SEMICOLON,
    PERIOD, 
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
        case TokenType::KW_CREATE:              return "KW_CREATE";
        case TokenType::KW_TABLE:               return "KW_TABLE";
        case TokenType::KW_INDEX:               return "KW_INDEX";
        case TokenType::KW_ON:                  return "KW_ON";
        case TokenType::KW_SELECT:              return "KW_SELECT";
        case TokenType::KW_WHERE:               return "KW_WHERE";
        case TokenType::KW_MODIFY:              return "KW_MODIFY";
        case TokenType::KW_SET:                 return "KW_SET";
        case TokenType::KW_SHOW:                return "KW_SHOW";
        case TokenType::KW_PRAGMA:              return "KW_PRAGMA";
        case TokenType::KW_INFO:                return "KW_INFO";
        case TokenType::KW_INSERT:              return "KW_INSERT";
        case TokenType::KW_INTO:                return "KW_INTO";
        case TokenType::KW_FROM:                return "KW_FROM";

        case TokenType::CHAR_32:                return "CHAR_32";
        case TokenType::CHAR_16:                return "CHAR_16";
        case TokenType::CHAR_8:                 return "CHAR_8";
        case TokenType::INT:                    return "INT";
        case TokenType::TEXT:                   return "TEXT";
        case TokenType::BOOL:                   return "BOOL";

        case TokenType::BRACKET_OPEN:           return "BRACKET_OPEN";
        case TokenType::BRACKET_CLOSE:          return "BRACKET_CLOSE";
        case TokenType::FLAG_INDEXED:           return "FLAG_INDEXED";
        case TokenType::FLAG_PRIMARY:           return "FLAG_PRIMARY";

        case TokenType::AND:                    return "AND";
        case TokenType::OR:                     return "OR";
        case TokenType::LESS_THAN:              return "LESS_THAN";
        case TokenType::GREATER_THAN:           return "GREATER_THAN";
        case TokenType::EQUALS:                 return "EQUALS";
        case TokenType::EQUAL_OR_LESS_THAN:     return "EQUAL_OR_LESS_THAN";
        case TokenType::EQUAL_OR_GREATER_THAN:  return "EQUAL_OR_GREATER_THAN";

        case TokenType::LITERAL:                return "LITERAL";
        case TokenType::IDENTIFIER:             return "IDENTIFIER";
        case TokenType::BOOL_LITERAL:             return "BOOL_LITERAL";

        case TokenType::ASTERISK: return "ASTERISK";

        case TokenType::OPENING_PARENTHESIS:    return "OPENING_PARENTHESIS";
        case TokenType::CLOSING_PARENTHESIS:    return "CLOSING_PARENTHESIS";
        case TokenType::QUOTE:                  return "QUOTE";
        case TokenType::SEMICOLON:              return "SEMICOLON";
        case TokenType::COMMA:                  return "COMMA";
        case TokenType::PERIOD:                 return "PERIOD";
        case TokenType::SET:                    return "SET";
    }
    return "UNKNOWN";
}

inline TokenType StringToTokenType(const std::string& text) {
    static const std::unordered_map<std::string, TokenType> map = {
        {"create",  TokenType::KW_CREATE},
        {"table",   TokenType::KW_TABLE},
        {"index",   TokenType::KW_INDEX},
        {"from",    TokenType::KW_FROM},
        {"on",      TokenType::KW_ON},
        {"select",  TokenType::KW_SELECT},
        {"where",   TokenType::KW_WHERE},
        {"*", TokenType::ASTERISK},
        {"modify",  TokenType::KW_MODIFY},
        {"set",     TokenType::KW_SET},
        {"show",    TokenType::KW_SHOW},
        {"pragma",  TokenType::KW_PRAGMA},
        {"info",    TokenType::KW_INFO},
        {"insert",  TokenType::KW_INSERT},
        {"into",    TokenType::KW_INTO},

        {"and",     TokenType::AND},
        {"or",      TokenType::OR},

        {"char32",  TokenType::CHAR_32},
        {"char16",  TokenType::CHAR_16},
        {"char8",   TokenType::CHAR_8},
        {"int",     TokenType::INT},
        {"text",  TokenType::TEXT},
        {"bool",    TokenType::BOOL},

        {"[",       TokenType::BRACKET_OPEN},
        {"]",       TokenType::BRACKET_CLOSE},
        {"indexed", TokenType::FLAG_INDEXED},
        {"primary", TokenType::FLAG_PRIMARY},

        {"(",       TokenType::OPENING_PARENTHESIS},
        {")",       TokenType::CLOSING_PARENTHESIS},
        {"\"",      TokenType::QUOTE},
        {";",       TokenType::SEMICOLON},
        {",",       TokenType::COMMA},
        {".",       TokenType::PERIOD},
        {"=",       TokenType::SET},

        {"<",       TokenType::LESS_THAN},
        {">",       TokenType::GREATER_THAN},
        {"==",      TokenType::EQUALS},
        {"<=",      TokenType::EQUAL_OR_LESS_THAN},
        {">=",      TokenType::EQUAL_OR_GREATER_THAN}
    };

    auto it = map.find(text);
    if (it != map.end())
        return it->second;

    // Anything not found is either a literal (if quoted later) or identifier
    return TokenType::IDENTIFIER;
}