#pragma once
#include <string>
#include "../core/database.hpp"
#include "tokens.hpp"
#include "ast.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "validator.hpp"

class Executor
{

private:
    Database &database;

public:
    Executor (Database &database);

    void execute (const std::string &input);
    
    void execute_insert(AST::InsertQuery* query);
    void execute_select(AST::FindQuery* query);
    void execute_create_table(AST::CreateTableQuery* query);

    void execute_create_index(AST::InsertQuery* query);
    void execute_create_index(AST::CreateIndexQuery* query);
};