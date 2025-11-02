#include "query.hpp"
#include <cstddef> // for std::size_t


arg::arg(std::string column, std::string rhs)
    : column(std::move(column)), rhs(std::move(rhs)){}


namespace CommandUtil {
    Command string_to_command(std::string input)
    {
        std::transform(input.begin(), input.end(), input.begin(), ::toupper);
        if (input == "CREATE")  return Command::CREATE;
        if (input == "INSERT")  return Command::INSERT;
        if (input == "INDEX")   return Command::INDEX;
        if (input == "FIND")    return Command::FIND;
        return Command::None;
    }
}

std::string errorToString(QueryError e)
{
    switch (e) {
        case QueryError::None:                  return "Success";
        case QueryError::NoCommand:             return "No command given";
        case QueryError::NotEnoughArgs:         return "Too few arguments";
        case QueryError::ArgColumnMismatch:     return "Provided argument doesn't match column";
        case QueryError::NoTable:               return "Table name is missing";
        case QueryError::InvalidTable:          return "Table does not exist";
        case QueryError::NoArgs:                return "No columns specified";
        case QueryError::InvalidType:           return "Invalid column type";
        case QueryError::ColumnAlreadyExists:   return "Column already created";
    }
    return "Unknown error";
}

QueryResult::QueryResult(QueryError error, std::string message)
    : error(error), message(std::move(message))
{
}


namespace Query
{
    void query_return(Command command, QueryResult result)
    {
        if(result.error != QueryError::None)
        {
            //if error
            std::cout << "ERROR: " << errorToString(result.error) << "\n";
            return;
        }

        std::cout << "SUCCSESS"  << "\n";

    }
    Args arg_vec_split(const StringVec &tokens)
    {
        Args output;
        for (auto &str: tokens)
        {
            __SIZE_TYPE__ split_index = str.find('=');
            if(split_index != std::string::npos)
            {
                std::string arg1 = std::string(str.begin(),str.begin() + split_index);
                std::string arg2 = std::string(str.begin() + split_index + 1, str.end());


                arg temp_arg = arg(arg1, arg2);
                
                output.push_back(temp_arg);
            }
        }
        return output;

    }

    void insertINTEGER(MyBtree4 *indexTree, Record &record, std::string primaryKey, File &file)
    {
        file.insert_data<MyBtree4,Node4,LeafNode4,InternalNode4>
        (primaryKey,record,*indexTree);
    }
    void insertSTRING(MyBtree32 *indexTree, Record &record, std::string primaryKey, File &file)
    {
        file.insert_data<MyBtree32,Node32,LeafNode32,InternalNode32>
        (primaryKey,record,*indexTree);    
    }
    /*
        create users (uid=int, username=string, password=int);
        insert into users (1, "smartpotato", 1234);
        find users where (username="smartpotato");
    */
    void execute(const std::string &input,File &file, 
    MyBtree32 *IndexTree32, 
    MyBtree8 *IndexTree8, 
    MyBtree4*IndexTree4)
    {
        StringVec tokens;
        tokenize(input, tokens);

        if(tokens.size() < 3) return;

        Command command = CommandUtil::string_to_command(tokens[0]);
        std::string table_name;

        QueryResult result;


        auto catch_error = [&](QueryError error)
        {
            result.error = error;
            query_return(command, result);
        };
        
        switch (command){
            case Command::CREATE: {                 //TODO: check for duplicate column name, table name 
                table_name = tokens[1];
                
                Table new_table;
                new_table.name = table_name;

                Args args = arg_vec_split(tokens);

                for (auto &arg : args)
                {
                    Type type = TypeUtil::string_to_type(arg.rhs);
                    if(!validate_type(type))                               { catch_error(QueryError::InvalidType); return; }

                    Column new_column(arg.column,type);

                    new_table.columns.push_back(new_column);
                }

                file.insert_table(&new_table); 

                result.error = QueryError::None;

                query_return(command,result); return;
            }
            case Command::FIND: {
                table_name = tokens[1];
                if(!validate_table(table_name, file.primary_table))        { catch_error(QueryError::InvalidTable); return; }

                std::string &search_column = tokens[3];
                std::string &search_word   = tokens[4];
                


            }
            case Command::INSERT: {
                table_name = tokens[2];
                if(!validate_table(table_name, file.primary_table))        { catch_error(QueryError::InvalidTable); return; }
                if(!validate_insert_length(tokens, file.primary_table))    { catch_error(QueryError::NotEnoughArgs); return; }


                int primary_key_len = TypeUtil::type_len(file.primary_table.columns[0].type);
                Type primaryKeyType = file.primary_table.columns[0].type;
                const std::string &primary_key = tokens[3]; 

                Record record(tokens,file.primary_table);

                if (record.length == -1) { catch_error(QueryError::ArgColumnMismatch); return;}

                switch (primaryKeyType){
                    case Type::INTEGER:
                        insertINTEGER(IndexTree4, record, primary_key, file);
                    case Type::STRING:
                        insertSTRING(IndexTree32, record, primary_key, file);
                }
            }
            case Command::None: {
                result.error = QueryError::NoCommand;
                break;
            }
        }
    }
}