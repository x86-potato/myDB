#pragma once

#include "../../config.h"
#include "../ast.hpp"

#include <string>
#include <vector>
#include <variant>
#include <iostream>
#include <stdexcept>

// Forward declarations
struct Predicate;
struct Boolean;
struct Path;

struct Boolean
{
    AST::Op op;
    Predicate* next_predicate = nullptr;
};


struct ColumnOperand
{
    std::string table;
    std::string column;
};

struct LiteralOperand
{
    std::string literal;
};


using Operand = std::variant<LiteralOperand, ColumnOperand>;

struct Predicate
{
    AST::Op op;
    Operand left;
    Operand right;

    enum class PredicateKind {
        LiteralSelection,  // column == literal
        ColumnSelection,   // column == column (same table)
        Join               // column == column (different tables)
    } predicate_kind;


};

struct Path
{
    std::vector<Predicate> predicates;
};

class Plan
{
public:
    using Paths = std::vector<Path>;
    Paths paths;

    explicit Plan(const AST::SelectQuery& query);

    void build_paths(AST::Expr* expr, Paths& paths);
    Predicate make_predicate(AST::Expr* expr);

    void debug_print_predicate(const Predicate& p);
    void debug_print_path(const Path& path, size_t index);
    void debug_print_plan(const Plan& plan);

    void execute();
};
