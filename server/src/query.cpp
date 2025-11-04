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
    std::string commandToString(Command command)
    {
        switch (command)
        {
            case Command::CREATE: return "CREATE";
            case Command::INSERT: return "INSERT";
            case Command::INDEX: return "INDEX";
            case Command::FIND: return "FIND";
            case Command::None: return "COMMAND ERROR";
        }
        return "NONE";
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
        case QueryError::SearchColumnNotFound:  return "Given search column does not exist";
        case QueryError::InsertFromFailed:      return "Given columns do not much the table give";
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
            std::cout << CommandUtil::commandToString(command) << " ERROR: " << errorToString(result.error) << "\n";
            return;
        }

        
        //std::cout << CommandUtil::commandToString(command) << " SUCCESS" << "\n";

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
        if(!validate_INTEGER_token(primaryKey)){return;}

        int32_t string_to_int = stoi(primaryKey);
        std::string converted(4, '\0');
        memcpy(&converted[0], &string_to_int, 4);

        file.insert_data<MyBtree4,Node4,LeafNode4,InternalNode4>
        (converted,record,*indexTree);
    }
    void insertSTRING(MyBtree32 *indexTree, Record &record, std::string primaryKey, File &file)
    {
        file.insert_data<MyBtree32,Node32,LeafNode32,InternalNode32>
        (primaryKey,record,*indexTree);    


    }
    void handleInsertFrom(const StringVec& tokens)
    {

        
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
                if(tokens.size() != 4) { catch_error(QueryError::NotEnoughArgs); return;}
                table_name = tokens[1];
                if(!validate_table(table_name, file.primary_table))        { catch_error(QueryError::InvalidTable); return; }

                Args arg = arg_vec_split(tokens);
                std::string &search_column = arg[0].column;
                std::string &search_word   = arg[0].rhs;
                
                if(!validatePrimaryFind(file.primary_table, search_column)) { catch_error(QueryError::SearchColumnNotFound);return;}
  
                switch (file.primary_table.columns[0].type)
                {
                    case Type::INTEGER:
                    {
                        if(!validate_INTEGER_token(search_word)) return;
                        int string_to_int = stoi(search_word);
                        std::string converted(4, '\0');
                        memcpy(&converted[0], &string_to_int, 4);
                        Record record = file.find<MyBtree4, Node4, InternalNode4, LeafNode4>(converted, *IndexTree4);
                        break;
                    }
                    case Type::STRING:
                    {
                        Record record = file.find<MyBtree32, Node32, InternalNode32, LeafNode32>(search_word, *IndexTree32);
                        break;
                    }
                    case Type::UNKNOWN:
                    {
                        break;
                    }

                }

                result.error = QueryError::None;
                query_return(command,result); return;
            }
            case Command::INSERT: {
                table_name = tokens[2];
                if(!validate_table(table_name, file.primary_table))        { catch_error(QueryError::InvalidTable); return; }
                if(tokens[3] != "from" && !validate_insert_length(tokens, file.primary_table))    { catch_error(QueryError::NotEnoughArgs); return; }

                if (tokens[3] == "from"){   //handle csv insertion
                    if (tokens.size() != 5) return;

                    std::string fileName = tokens[4];
                    std::ifstream dataFile(strip_quotes(fileName));

                    std::string line;
                    std::getline(dataFile, line);

                    StringVec headerTokens;
                    tokenize(line, headerTokens);

                    Type primaryKeyType = file.primary_table.columns[0].type;

                    if(!validateColumnList(file.primary_table, headerTokens)) { catch_error(QueryError::InsertFromFailed); return;}

                    while (std::getline(dataFile, line))
                    {
                        StringVec insertLine;
                        tokenize(line, insertLine);

                        Record record(insertLine, file.primary_table, 0);

                        if (record.length == -1) { catch_error(QueryError::ArgColumnMismatch); return;}
                        
                        switch (primaryKeyType){
                            case Type::INTEGER:
                                
                                insertINTEGER(IndexTree4, record, insertLine[0], file);
                                break;
                            case Type::STRING:
                                insertSTRING(IndexTree32, record, insertLine[0], file);
                                break;

                            case Type::UNKNOWN:
                                break;
                        }

                    }


                    return;
                }
                
                //handle standrd insertion
                int primary_key_len = TypeUtil::type_len(file.primary_table.columns[0].type);
                Type primaryKeyType = file.primary_table.columns[0].type;
                const std::string &primary_key = tokens[3]; 

                Record record(tokens,file.primary_table, 3);

                if (record.length == -1) { catch_error(QueryError::ArgColumnMismatch); return;}

                switch (primaryKeyType){
                    case Type::INTEGER:
                        
                        insertINTEGER(IndexTree4, record, primary_key, file);
                        break;
                    case Type::STRING:
                        insertSTRING(IndexTree32, record, primary_key, file);
                        break;

                    case Type::UNKNOWN:
                        break;
                }
                result.error = QueryError::None;
                query_return(command,result); return;
            }
            case Command::INDEX: {
                return;
            }
            case Command::None: {
                result.error = QueryError::NoCommand;

                catch_error(QueryError::NoCommand);
            }
        }
    }
}