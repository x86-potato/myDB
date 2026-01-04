#pragma once
#include <string>
#include <variant>
#include "../config.h"
#include "tokens.hpp"
#include <memory>
#include <variant>

namespace AST
{  
    struct LogicalExpr; 
    enum class QueryType
    {
        CreateTable,
        CreateIndex,
        Insert,
        Select,
        Load,
        Run
    };

    struct Query
    {
        QueryType type;
        virtual ~Query() = default;
    };


    enum class Op {
        AND,
        OR,
        EQ,
        NEQ,
        LT,
        GT,
        LTE,
        GTE,
        ERROR
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
        std::string table;
        std::string column;
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
    struct SelectQuery : Query
    {
        StringVec tableNames;
        bool has_where = false;
        Condition condition;
    };
    struct LoadQuery : Query
    {
        string tableName;
        string fileName;
    };
    struct RunQuery : Query
    {
        string fileName;
    };

    inline int precedence(const Op &op)
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
    inline Op token_to_op(const Token& tok)
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
            default:                               return Op::ERROR;
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



    // Recursive helper to print an AST::Expr tree
    inline void print_expr_tree(const Expr& expr, int indent = 0) {
        auto print_indent = [indent]() {
            for (int i = 0; i < indent; ++i) std::cout << "  ";
        };

        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, ColumnRef>) {
                print_indent();
                std::cout << "Table: " << arg.table << "\n";
                print_indent();
                std::cout << "Column: " << arg.column << "\n";

            } else if constexpr (std::is_same_v<T, Literal>) {
                print_indent();
                std::cout << "Literal: \"" << arg.value << "\"\n";

            } else if constexpr (std::is_same_v<T, std::unique_ptr<LogicalExpr>>) {
                if (!arg) {
                    print_indent();
                    std::cout << "[Empty LogicalExpr]\n";
                    return;
                }
                print_indent();
                std::cout << "LogicalExpr: " << op_to_string(arg->op) << "\n";

                if (arg->left) {
                    print_expr_tree(*arg->left, indent + 1);
                } else {
                    print_indent();
                    std::cout << "  [Left nullptr]\n";
                }

                if (arg->right) {
                    print_expr_tree(*arg->right, indent + 1);
                } else {
                    print_indent();
                    std::cout << "  [Right nullptr]\n";
                }

            } else if constexpr (std::is_same_v<T, exprError>) {
                print_indent();
                std::cout << "[Error Expr]\n";
            }
        }, expr);
    }

    // Print the full SelectQuery tree
    inline void print_select_query_tree(const SelectQuery& query) {
        for (const auto& table : query.tableNames)
        {
            std::cout << "Table: " << table << "\n";
        }

        if (!query.has_where || !query.condition.root) {
            std::cout << "No WHERE clause.\n";
            return;
        }

        std::cout << "WHERE clause AST:\n";
        print_expr_tree(*query.condition.root, 1);
    }
};