#pragma once
#include <string>
#include <variant>
#include "../config.h"
#include "tokens.hpp"
#include <memory>

namespace AST
{  
    struct LogicalExpr; 
    enum class QueryType
    {
        CreateTable,
        CreateIndex,
        Insert,
        Find
    };

    struct Query
    {
        QueryType type;
    };


    enum class Op {
        AND,
        OR,
        EQ,
        NEQ,
        LT,
        GT,
        LTE,
        GTE
    };

    using string = std::string;


    struct CreateArg
    {
        std::string column;
        std::string type;
    };
    struct InsertArg
    {
        string value;
    };
    

    struct ColumnRef 
    { 
        std::string name;
    };                                   

    struct Literal   
    { 
        string value;
    };
    struct exprError{};

    using Expr = std::variant<
    ColumnRef, Literal, std::unique_ptr<LogicalExpr>,
    exprError 
    >;

    struct LogicalExpr
    {
        Op op;
        std::unique_ptr<Expr> left;
        std::unique_ptr<Expr> right;
    };

    struct Condition
    {
        std::unique_ptr<Expr> root;
    };





    struct CreateTableQuery : Query
    {
        string tableName;
        std::vector<CreateArg> args;
    };
    struct CreateIndexQuery : Query
    {
        string tableName;
        string column;
    };
    struct InsertQuery : Query
    {
        string tableName;
        std::vector<InsertArg> args;
    };
    struct FindQuery : Query
    {
        string tableName;
        Condition condition;
    };

    inline const int precedence(const Op &op)
    {
        switch(op)
        {
            case Op::EQ:   // ==
            case Op::NEQ:  
            case Op::LT:   // <
            case Op::GT:   // >
            case Op::LTE:  // <=
            case Op::GTE:  // >=
                return 3; // Comparisons bind tighter than logical operators

            case Op::AND:
                return 2; // AND binds tighter than OR

            case Op::OR:
                return 1; // OR has lowest precedence

            default:
                return 0; // Unknown operators, lowest precedence
        }
    }
    inline bool is_operator(const Token* tok) {
        if (!tok) return false;

        switch (tok->type) {
            case TokenType::AND:
            case TokenType::OR:
            case TokenType::EQUALS:
            case TokenType::LESS_THAN:
            case TokenType::GREATER_THAN:
            case TokenType::EQUAL_OR_LESS_THAN:
            case TokenType::EQUAL_OR_GREATER_THAN:
                return true;

            default:
                return false;
        }
    }
    inline const Op token_to_op(const Token& tok)
    {
        switch(tok.type)
        {
            case TokenType::AND:                   return Op::AND;
            case TokenType::OR:                    return Op::OR;
            case TokenType::EQUALS:                return Op::EQ;
            case TokenType::LESS_THAN:             return Op::LT;
            case TokenType::GREATER_THAN:          return Op::GT;
            case TokenType::EQUAL_OR_LESS_THAN:    return Op::LTE;
            case TokenType::EQUAL_OR_GREATER_THAN: return Op::GTE;
        }

        return Op::NEQ;
    }
    inline const char* op_to_string(const AST::Op op)
    {
        switch(op)
        {
            case AST::Op::AND:  return "AND";
            case AST::Op::OR:   return "OR";
            case AST::Op::EQ:   return "==";
            case AST::Op::NEQ:  return "!=";
            case AST::Op::LT:   return "<";
            case AST::Op::GT:   return ">";
            case AST::Op::LTE:  return "<=";
            case AST::Op::GTE:  return ">=";
            default:            return "?";
        }
    }
};