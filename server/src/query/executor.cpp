#include "executor.hpp"


Executor::Executor (Database &database) : database(database)
{


}


void Executor::execute (const std::string &input)
{
    std::vector<Token> tokenList;
    std::unique_ptr<AST::Query> queryAST;

    Lexer::tokenize(input, tokenList);


    ParserNameSpace::Parser parser;
    ParserNameSpace::ParserReturn output = parser.parse(tokenList);
    queryAST = std::move(output.queryPointer);

    if (output.code != 0) return;

    switch (queryAST->type) {
        case AST::QueryType::CreateTable:
            execute_create_table(
                static_cast<AST::CreateTableQuery*>(queryAST.get())
            );
            break;
            
        case AST::QueryType::Insert:
            execute_insert(
                static_cast<AST::InsertQuery*>(queryAST.get())
            );
            break;
            
        case AST::QueryType::Select:
            //AST::print_select_query_tree(
            //    *(static_cast<AST::SelectQuery*>(queryAST.get()))
            //);
            execute_select(
                static_cast<AST::SelectQuery*>(queryAST.get())
            );
            break;
        case AST::QueryType::Load:
            execute_load(
                static_cast<AST::LoadQuery*>(queryAST.get())
            );
            break;        
        case AST::QueryType::Run:
            execute_run(
                static_cast<AST::RunQuery*>(queryAST.get())
            );
            break;    
        case AST::QueryType::CreateIndex:
            execute_create_index(
                static_cast<AST::CreateIndexQuery*>(queryAST.get())
            );
            break;
            
    }
}

void Executor::execute_create_index(AST::CreateIndexQuery* query) {
    if (validateCreateIndexQuery(*query, database) == false)
    {
        return;
    }

    Table &table= database.tableMap.at(query->tableName);

    int columnIndex = -1;
    for (size_t i = 0; i < table.columns.size(); ++i)
    {
        if (table.columns[i].name == query->column)
        {
            columnIndex = i;
            break;
        }
    }
    //should not happen due to prior validation
    if (columnIndex == -1)
    {
        std::cout << "Column " << query->column << " not found in table " << query->tableName << std::endl;
        return;
    }

    std::cout << "Generating index for column " << query->column << " in table " << query->tableName << std::endl;
    

    database.file->generate_index<MyBtree32, MyBtree16, MyBtree8, MyBtree4>(columnIndex, table,
        &database.index_tree32,
        &database.index_tree16,
        &database.index_tree8,
        &database.index_tree4
    );
}



void Executor::execute_select(AST::SelectQuery* query) {
    //AST::print_select_query_tree(*query);

    Plan plan(*query);
    if (validatePlan(plan, database) == false)
    {
       return;
    }
    //plan.debug_print_plan(plan);


    if(plan.paths.size() == 0)
    {
        Path temp = Path{}; 
        Pipeline plan_executor(temp, database);

        plan_executor.Execute();
    }
    else
    {
        Pipeline plan_executor(plan.paths[0], database);

        plan_executor.Execute();
    }

}

void Executor::execute_load(AST::LoadQuery* query) {
    // Debug: print current working directory
    std::string filename = query->fileName;
    if (filename.front() == '"' && filename.back() == '"') {
        filename = filename.substr(1, filename.length() - 2);
    }

    // Open the CSV file
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename  << std::endl;
        return;
    }

    std::string line;
    int lineNumber = 0;

    // Skip the first line (header)
    std::getline(file, line);
    lineNumber++;

    // Process each data line
    while (std::getline(file, line)) {
        lineNumber++;

        // Skip empty lines
        if (line.empty()) continue;

        // Parse CSV line into values
        StringVec values;
        std::stringstream ss(line);
        std::string value;

        while (std::getline(ss, value, ',')) {
            // Trim whitespace
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);

            for (auto &val : values) {
                std::cout << val << " | ";
            }
            
            values.push_back(value);
        }

        // Insert the row into the database
        try {
            database.insert(query->tableName, values);
        } catch (const std::exception& e) {
            std::cerr << "Error inserting row " << lineNumber << ": " << e.what() << std::endl;
        }
    }

    file.close();
    std::cout << "Successfully loaded " << (lineNumber - 1) << " rows into table " 
              << query->tableName << std::endl;
}

void Executor::execute_create_table(AST::CreateTableQuery* query) {
    if (validateCreateTableQuery(*query, database) == false)
    {
        return;
    }

    //if here, the query is valid and may be applied to database with no chance of conflict
    Table newTable;
    newTable.name = query->tableName;
    
    for (auto& arg : query->args)
    {
        Type type = TypeUtil::string_to_type(arg.type);
        Column newColumn(arg.column, type);

        newTable.columns.push_back(newColumn);
    }

    database.file->insert_table<Node32, Node16, Node8, Node4>(newTable);
    database.tableMap.insert({query->tableName, newTable});
}

void Executor::execute_insert(AST::InsertQuery* query) {
    if (validateInsertQuery(*query, database) == false)
    {
        return;
    }

    StringVec values;
    
    for (auto &arg: query->args)
    {
        values.push_back(arg.value);
    }

    std::string tableName = query->tableName;

    database.insert(tableName, values);

}

void Executor::execute_run(AST::RunQuery* query) {
    std::string filename = query->fileName;
    if (filename.front() == '"' && filename.back() == '"') {
        filename = filename.substr(1, filename.length() - 2);
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Error: Could not open file " << filename  << std::endl;
        return;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Parse multiple queries separated by semicolons
    size_t start = 0;
    size_t end = content.find(';');
    
    while (end != std::string::npos) {
        std::string query_str = content.substr(start, end - start);
        
        // Trim whitespace
        query_str.erase(0, query_str.find_first_not_of(" \t\r\n"));
        query_str.erase(query_str.find_last_not_of(" \t\r\n") + 1);
        
        if (!query_str.empty()) {
            execute(query_str + ";");
        }
        
        start = end + 1;
        end = content.find(';', start);
    }
}