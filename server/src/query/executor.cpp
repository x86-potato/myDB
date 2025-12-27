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
            execute_select(
                static_cast<AST::SelectQuery*>(queryAST.get())
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
    plan.debug_print_plan(plan);



    Pipeline plan_executor(plan.paths[0], database);

    plan_executor.Execute();
    //using Cursor32 = <MyBtree32>;
    //database.index_tree32.root_node = database.file->load_node<typename MyBtree32::NodeType>(index_location);
    //BPlusTreeCursor<MyBtree32> cursor(&database.index_tree32, plan.paths[0].predicates[0].right);    

    //display_results(results);
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