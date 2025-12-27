#pragma once
#include <string>
#include "../core/database.hpp"
#include "../core/cursor.hpp"
#include "plan/builder.hpp"
#include "tokens.hpp"
#include "ast.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "validator.hpp"
#include "plan/planner.hpp"

class Executor
{

private:
    Database &database;

public:
    Executor (Database &database);

    void execute (const std::string &input);
    
    void execute_insert(AST::InsertQuery* query);
    void execute_select(AST::SelectQuery* query);
    void execute_create_table(AST::CreateTableQuery* query);

    void execute_create_index(AST::InsertQuery* query);
    void execute_create_index(AST::CreateIndexQuery* query);
};