#pragma once

#include "../../core/database.hpp"
#include "../../core/cursor.hpp"
#include "planner.hpp"
#include "../executor.hpp"
#include "../validator.hpp"

#include "../ast.hpp"
#include <unordered_set>

// Forward declaration
class PathExecutor;

class PlanExecutor {
public:
    PlanExecutor(Database& db, const Plan& plan);

    // Executes the full plan and returns resulting RowIDs
    std::vector<off_t> execute();

private:
    Database& database;
    const Plan& plan;

    // Optional: store already seen rows for OR path deduplication
    std::unordered_set<off_t> seen_rows;
};


class PathExecutor {
public:
    PathExecutor(Database& db, const Path& path);

    // Returns true if next row exists, outputs RowID
    bool next(off_t& out);

private:
    Database& database;
    const Path& path;

    // Internal state for cursors / index iteration
};
