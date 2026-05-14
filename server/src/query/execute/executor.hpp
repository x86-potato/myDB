#pragma once
#include <string>
#include <cassert>
#include <chrono>

#include "../../core/database.hpp"
#include "../../core/cursor.hpp"
#include "../plan/physical.hpp"
#include "../parse/tokens.hpp"
#include "../parse/ast.hpp"
#include "../parse/lexer.hpp"
#include "../parse/parser.hpp"
#include "../validator.hpp"
#include "../plan/logical.hpp"
#include "../../server/server.hpp"


class Server;

class Executor
{

private:
    Database &database;

public:
    Executor (Database &database);

    void execute (const std::string &input, Session& session);

    void execute_create_table(AST::CreateTableQuery* query, Session& session);
    void execute_insert(AST::InsertQuery* query, Session& session);
    void execute_delete(AST::DeleteQuery* query, Session& session);
    void execute_select(AST::SelectQuery* query, Session& session);
    void execute_update(AST::UpdateQuery* query, Session& session);
    void execute_load(AST::LoadQuery* query, Session& session);
    void execute_run(AST::RunQuery* query, Session& session);
    void execute_create_index(AST::CreateIndexQuery* query);
    void execute_show(AST::ShowQuery* query);

    void execute_begin(AST::BeginQuery* query, Session& session);
    void execute_commit(AST::CommitQuery* query, Session& session);
    void execute_rollback(AST::RollbackQuery* query, Session& session);

};
