#pragma once

#include <string>
#include <vector>
#include "../config.h"
#include <cstddef>
#include <unordered_set>
#include "ast.hpp"
#include "../core/database.hpp"
#include "plan/planner.hpp"
#include "../core/table.hpp"

class Database;

bool validatePlan(const Plan& plan, const Database &db);
bool validateCreateIndexQuery(const AST::CreateIndexQuery &query, const Database &db);
bool validateInsertQuery(const AST::InsertQuery &query, const Database &db);
bool validateCreateTableQuery(const AST::CreateTableQuery &query, const Database &db);
bool validateDeleteQuery(const AST::DeleteQuery &query, const Database &db);
