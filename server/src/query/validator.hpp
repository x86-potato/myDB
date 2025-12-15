#pragma once

#include <string>
#include <vector>
#include "../config.h"
#include <cstddef>
#include <unordered_set>
#include "ast.hpp"
#include "../core/database.hpp"

class Database;

bool validateCreateIndexQuery(const AST::CreateIndexQuery &query, const Database &db);
bool validateInsertQuery(const AST::InsertQuery &query, const Database &db);
bool validateCreateTableQuery(const AST::CreateTableQuery &query, const Database &db);

