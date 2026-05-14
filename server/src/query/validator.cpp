#include "validator.hpp"

void throwError(const char *msg)
{
    std::cout << "VALIDATOR ERROR: " << msg << "\n";
}

bool validateInt(const std::string& str)
{
    if (str.empty())
        return false;

    const char* INT_MAX_STR = "2147483647";
    const char* INT_MIN_STR = "2147483648";

    bool negative = false;
    size_t start = 0;

    if (str[0] == '-') {
        negative = true;
        start = 1;
    }

    // must contain digits after optional '-'
    if (start == str.size())
        return false;

    // digit-only check
    for (size_t i = start; i < str.size(); ++i) {
        if (str[i] < '0' || str[i] > '9')
            return false;
    }

    size_t digitCount = str.size() - start;

    // max digit counts
    if (digitCount > 10)
        return false;

    if (digitCount < 10)
        return true;

    const std::string& limit = negative ? INT_MIN_STR : INT_MAX_STR;

    for (size_t i = 0; i < 10; ++i) {
        if (str[start + i] < limit[i])
            return true;
        if (str[start + i] > limit[i])
            return false;
    }

    return true; // exact match
}


int indexOfColumn(const std::string& name, const Table &table)
{
    for (size_t i = 0; i < table.columns.size(); i++)
    {
        if(name == table.columns[i].name)
            return int(i);
    }
    return -1; //return -1 if not found at all

}
bool checkIfTableContainsColumn(const Table& table, const std::string& columnName)
{
    for (const auto& col : table.columns)
    {
        if (col.name == columnName)
        {
            return true;
        }
    }
    return false;
}

bool validateLiteralSelectionPredicate(const Predicate& predicate, const Database &db, std::string* out_error)
{
    //case where users.id == literal
    auto left_table = std::get<ColumnOperand>(predicate.left).table;
    if (db.tableMap.find(left_table) == db.tableMap.end())
    {
        std::string output = "Table " + left_table + " does not exist!";
        if (out_error) *out_error = output;
        throwError(output.c_str());
        return false;
    }
    if (!checkIfTableContainsColumn(db.tableMap.at(left_table), std::get<ColumnOperand>(predicate.left).column))
    {
        std::string output = "Column " + std::get<ColumnOperand>(predicate.left).column + " does not exist in table " + left_table;
        if (out_error) *out_error = output;
        throwError(output.c_str());
        return false;
    }

    return true;
}
bool validateColumnSelectionPredicate(const Predicate& predicate, const Database &db, std::string* out_error)
{
    auto left_table = std::get<ColumnOperand>(predicate.left).table;
    if (db.tableMap.find(left_table) == db.tableMap.end())
    {
        std::string output = "Table " + left_table + " does not exist!";
        if (out_error) *out_error = output;
        throwError(output.c_str());
        return false;
    }
    auto right_table = std::get<ColumnOperand>(predicate.right).table;
    if (db.tableMap.find(right_table) == db.tableMap.end())
    {
        std::string output = "Table " + right_table + " does not exist!";
        if (out_error) *out_error = output;
        throwError(output.c_str());
        return false;
    }
    if (left_table != right_table)
    {
        std::string output = "In multi-table filter, both columns cannot be from the same table " + left_table;
        if (out_error) *out_error = output;
        throwError(output.c_str());
        return false;
    }
    if (!checkIfTableContainsColumn(db.tableMap.at(left_table), std::get<ColumnOperand>(predicate.left).column))
    {
        std::string output = "Column " + std::get<ColumnOperand>(predicate.left).column + " does not exist in table " + left_table;
        if (out_error) *out_error = output;
        throwError(output.c_str());
        return false;
    }
    if (!checkIfTableContainsColumn(db.tableMap.at(right_table), std::get<ColumnOperand>(predicate.right).column))
    {
        std::string output = "Column " + std::get<ColumnOperand>(predicate.right).column + " does not exist in table " + right_table;
        if (out_error) *out_error = output;
        throwError(output.c_str());
        return false;
    }

    return true;
}

bool validateJoinPredicate(const Predicate& predicate, const Database &db, std::string* out_error)
{
    auto left_table = std::get<ColumnOperand>(predicate.left).table;
    if (db.tableMap.find(left_table) == db.tableMap.end())
    {
        std::string output = "Table " + left_table + " does not exist!";
        if (out_error) *out_error = output;
        throwError(output.c_str());
        return false;
    }
    auto right_table = std::get<ColumnOperand>(predicate.right).table;
    if (db.tableMap.find(right_table) == db.tableMap.end())
    {
        std::string output = "Table " + right_table + " does not exist!";
        if (out_error) *out_error = output;
        throwError(output.c_str());
        return false;
    }

    if (left_table == right_table)
    {
        std::string output = "In join predicate, both columns cannot be from the same table " + left_table;
        if (out_error) *out_error = output;
        throwError(output.c_str());
        return false;
    }
    if (!checkIfTableContainsColumn(db.tableMap.at(left_table), std::get<ColumnOperand>(predicate.left).column))
    {
        std::string output = "Column " + std::get<ColumnOperand>(predicate.left).column + " does not exist in table " + left_table;
        if (out_error) *out_error = output;
        throwError(output.c_str());
        return false;
    }
    if (!checkIfTableContainsColumn(db.tableMap.at(right_table), std::get<ColumnOperand>(predicate.right).column))
    {
        std::string output = "Column " + std::get<ColumnOperand>(predicate.right).column + " does not exist in table " + right_table;
        if (out_error) *out_error = output;
        throwError(output.c_str());
        return false;
    }

    return true;
}



bool validateSelectQuery(const AST::SelectQuery &query, const Database &db, std::string* out_error)
{
    // 1. All table names must exist
    for (const auto& table_name : query.tableNames)
    {
        if (db.tableMap.find(table_name) == db.tableMap.end())
        {
            std::string msg = "Table '" + table_name + "' does not exist";
            if (out_error) *out_error = msg;
            throwError(msg.c_str());
            return false;
        }
    }

    // 2. Column list / aggregate column validation
    if (!query.select_all && query.aggregates.empty())
    {
        if (query.selected_columns.empty())
        {
            std::string msg = "SELECT has no columns and is not SELECT *";
            if (out_error) *out_error = msg;
            throwError(msg.c_str());
            return false;
        }
        for (const auto& sc : query.selected_columns)
        {
            std::string tname = sc.table.empty() ? query.tableNames[0] : sc.table;

            // table referenced in column must be in the FROM list
            bool in_from = false;
            for (const auto& t : query.tableNames)
                if (t == tname) { in_from = true; break; }
            if (!in_from)
            {
                std::string msg = "Table '" + tname + "' referenced in column list is not in FROM clause";
                if (out_error) *out_error = msg;
                throwError(msg.c_str());
                return false;
            }

            if (!checkIfTableContainsColumn(db.tableMap.at(tname), sc.column))
            {
                std::string msg = "Column '" + sc.column + "' does not exist in table '" + tname + "'";
                if (out_error) *out_error = msg;
                throwError(msg.c_str());
                return false;
            }
        }
    }
    else
    {
        for (const auto& item : query.aggregates)
        {
            if (item.type == AST::AggregateType::SUM ||
                item.type == AST::AggregateType::MAX ||
                item.type == AST::AggregateType::MIN)
            {
                const auto& sc = item.column;
                std::string tname = sc.table.empty() ? query.tableNames[0] : sc.table;

                bool in_from = false;
                for (const auto& t : query.tableNames)
                    if (t == tname) { in_from = true; break; }
                if (!in_from)
                {
                    std::string msg = "Table '" + tname + "' in aggregate is not in FROM clause";
                    if (out_error) *out_error = msg;
                    throwError(msg.c_str());
                    return false;
                }

                const Table& tbl = db.tableMap.at(tname);
                if (!checkIfTableContainsColumn(tbl, sc.column))
                {
                    std::string msg = "Column '" + sc.column + "' does not exist in table '" + tname + "'";
                    if (out_error) *out_error = msg;
                    throwError(msg.c_str());
                    return false;
                }

                // SUM/MAX/MIN require an INTEGER column
                for (const auto& col : tbl.columns)
                {
                    if (col.name == sc.column && col.type != Type::INTEGER)
                    {
                        std::string msg = "SUM/MAX/MIN require an INTEGER column, '" + sc.column + "' is not";
                        if (out_error) *out_error = msg;
                        throwError(msg.c_str());
                        return false;
                    }
                }
            }
        }
    }

    // 3. LIMIT must be positive when set
    if (query.limit == 0 || query.limit < -1)
    {
        std::string msg = "LIMIT value must be a positive integer";
        if (out_error) *out_error = msg;
        throwError(msg.c_str());
        return false;
    }

    return true;
}

//first we only
bool validatePlan(const LogicalPlan& plan, const Database &db, std::string* out_error)
{
    for (auto &path: plan.paths)
    {

        if(path.predicates.size() == 0)
            return true;
        for (auto &predicate: path.predicates)
        {
            switch (predicate.predicate_kind)
            {
                case Predicate::PredicateKind::LiteralSelection:
                    if (!validateLiteralSelectionPredicate(predicate, db, out_error))
                        return false;
                    break;
                case Predicate::PredicateKind::ColumnSelection:
                    if (!validateColumnSelectionPredicate(predicate, db, out_error))
                        return false;
                    break;
                case Predicate::PredicateKind::Join:
                    if (!validateJoinPredicate(predicate, db, out_error))
                        return false;
                    break;
            }

        }
    }

    return true;
}

bool validateCreateIndexQuery(const AST::CreateIndexQuery &query, const Database &db)
{
    // check if table name like this even exists
    if(db.tableMap.find(query.tableName) == db.tableMap.end())
    {
        throwError("This table does not exist!");
        return false;
    }
    // check if number of columns given in query is 1
    if(query.column.length() < 1)
    {
        throwError("No column given!");
        return false;
    }

    const Table &table = db.tableMap.at(query.tableName);
    //check if column exists
    bool column_found = false;
    for (const auto& col : table.columns)
    {
        if (col.name == query.column)
        {
            if (col.type == Type::TEXT)
            {
                throwError("TEXT columns cannot be indexed");
                return false;
            }
            column_found = true;
            break;
        }
    }
    if (!column_found)
    {
        throwError("This column does not exist!");
        return false;
    }
    return true;
}

bool validateChar(const std::string &str, size_t typelen)
{
    if(str.length() < 3) return false;
    if(str[0] != '"' || str[str.length()-1] != '"') return false;
    if(str.length()-2 > typelen) return false;
    return true;
}

bool validateInsertQuery(const AST::InsertQuery &query, const Database &db)
{
    // check if table name like this even exists
    if(db.tableMap.find(query.tableName) == db.tableMap.end())
    {
        throwError("This table does not exist!");
        return false;
    }
    //check if enough args given

    size_t columnCount = db.tableMap.at(query.tableName).columns.size();
    if(query.args.size() != columnCount)
    {
        throwError("Values given dont match schema!");
        return false;
    }

    for (size_t i = 0; i < columnCount; i++)
    {
        //std::cout << query.args[i].value.length();
        if(query.args[i].value.length() < 1)
        {

            std::string output = std::string("column " +
            db.tableMap.at(query.tableName).columns[i].name +
            " given value is too short");
            throwError(output.c_str());
            return false;
        }

        //switch thru the in memory table structure
        switch (db.tableMap.at(query.tableName).columns[i].type)
        {

            case Type::CHAR32:
            {
                if(!validateChar(query.args[i].value, 32)) {
                    std::string output = std::string("column " +
                    db.tableMap.at(query.tableName).columns[i].name +
                    " expects no more than 32 chars");
                    throwError(output.c_str());
                    return false;
                }
                break;
            };
            case Type::CHAR16:
            {
                if(!validateChar(query.args[i].value,16)) {
                    std::string output = std::string("column " +
                    db.tableMap.at(query.tableName).columns[i].name +
                    " expects no more than 16 chars");
                    throwError(output.c_str());
                    return false;
                }
                break;
            };
            case Type::CHAR8:
            {
                if(!validateChar(query.args[i].value, 8)) {
                    std::string output = std::string("column " +
                    db.tableMap.at(query.tableName).columns[i].name +
                    " expects no more than 8 chars");
                    throwError(output.c_str());
                    return false;
                }
                break;
            };
            case Type::INTEGER:
            {
                if(!validateInt(query.args[i].value))
                {
                    std::string output = std::string("column " +
                    db.tableMap.at(query.tableName).columns[i].name +
                    " expects a valid integer type");
                    throwError(output.c_str());
                    return false;
                }
                break;
            };
            case Type::BOOL:
            {
                if(query.args[i].value != "true" && query.args[i].value != "false")
                {
                    std::string output = std::string("column " +
                    db.tableMap.at(query.tableName).columns[i].name +
                    " expects a valid boolean type");
                    throwError(output.c_str());
                    return false;
                }
                break;
            };
            case Type::TEXT:
            {
                // TEXT is a quoted string, no fixed length limit (uint8 payload, max 255 chars)
                if (query.args[i].value.size() < 2 ||
                    query.args[i].value.front() != '"' ||
                    query.args[i].value.back()  != '"')
                {
                    std::string output = "column " +
                        db.tableMap.at(query.tableName).columns[i].name +
                        " expects a quoted string value";
                    throwError(output.c_str());
                    return false;
                }
                if (query.args[i].value.size() - 2 > 255)
                {
                    std::string output = "column " +
                        db.tableMap.at(query.tableName).columns[i].name +
                        " TEXT value exceeds 255 characters";
                    throwError(output.c_str());
                    return false;
                }
                break;
            };
        }
    }


    return true;
}


bool validateCreateTableQuery(const AST::CreateTableQuery &query, const Database &db)
{
    //check if table of that name exists
    if(db.tableMap.find(query.tableName) != db.tableMap.end())
    {
        throwError("Table of this name already exists!");
        return false;
    }

    //check if name is non empty
    if(query.tableName.length() < 2)
    {
        throwError("Table name too short!");
        return false;
    }
    //check if enough args given
    if(query.args.size() == 0)
    {
        throwError("No attributes assigned!");
        return false;
    }
    //check for duplicate columns
    std::unordered_set<std::string> seen;
    for (const auto& arg : query.args)
    {
        if (arg.column.empty())
        {
            throwError("Column name cannot be empty");
            return false;
        }

        if (arg.column.length() < 1 || arg.column.length() > MAX_COLUMN_NAME)
        {
            throwError("Invalid column name length");
            return false;
        }
        auto [it, inserted] = seen.insert(arg.column);
        if (!inserted)
        {
            throwError("Cant have duplicate column names!");
            return false;
        }
    }

    // First column is the primary index — it must be an indexable type
    const std::string& first_type = query.args[0].type;
    if (first_type == "text" || first_type == "bool")
    {
        throwError("First column must be an indexable type (int, char8, char16, char32) to serve as primary index");
        return false;
    }

    return true;
}
bool validateDeleteQuery(const AST::DeleteQuery &query, const Database &db)
{
    //check if table of that name exists
    if(db.tableMap.find(query.tableName) == db.tableMap.end())
    {
        throwError("Table of this name does not exist!");
        return false;
    }


    return true;
}
