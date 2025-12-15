#include "validator.hpp"
#include "../core/table.hpp"
#include "../core/database.hpp"

void throwError(const char *msg)
{
    std::cout << "VALIDATOR ERROR: " << msg << std::endl;
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

bool validateChar(const std::string &str, int typelen)
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

    int columnCount = db.tableMap.at(query.tableName).columns.size();
    if(query.args.size() != columnCount)
    {
        throwError("Values given dont match schema!");
        return false;
    }

    for (int i = 0; i < columnCount; i++)
    {
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



    return true;
}