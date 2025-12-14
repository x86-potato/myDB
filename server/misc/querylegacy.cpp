#include "querylegacy.hpp"
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
    off_t insertCHAR8(MyBtree8 *indexTree, Record &record, std::string primaryKey, File &file)
    {
        return file.insert_primary_index<MyBtree8>(strip_quotes(primaryKey), record, *indexTree);
    }
    off_t insertCHAR16(MyBtree16 *indexTree, Record &record, std::string primaryKey, File &file)
    {
        return file.insert_primary_index<MyBtree16>(strip_quotes(primaryKey), record, *indexTree);
    }
    off_t insertCHAR32(MyBtree32 *indexTree, Record &record, std::string primaryKey, File &file)
    {
        return file.insert_primary_index<MyBtree32>(strip_quotes(primaryKey), record, *indexTree);
    }
    off_t insertINTEGER(MyBtree4 *indexTree, Record &record, std::string primaryKey, File &file)
    {
        if(!validate_INTEGER_token(primaryKey)){return 0;}

        int32_t val = stoi(primaryKey);
        uint32_t be = htobe32(static_cast<uint32_t>(val));  // convert to big-endian
        std::string converted(4, '\0');
        memcpy(&converted[0], &be, 4);

        return file.insert_primary_index<MyBtree4>(converted, record, *indexTree);
    }
    off_t insertSTRING(MyBtree32 *indexTree, Record &record, std::string primaryKey, File &file)
    {
        //file.insert_data<MyBtree32,Node32,LeafNode32,InternalNode32>
        //(primaryKey,record,*indexTree);    

        return file.insert_primary_index<MyBtree32>(primaryKey, record, *indexTree);


    }
    void insertSecondary(MyBtree32 *indexTree32, MyBtree16 *indexTree16, MyBtree8 *indexTree8, MyBtree4 *indexTree4, 
    Record &record, std::string key, File &file, off_t record_location, int i, off_t index_location, Type type)
    {
        switch(type)
        {
            case Type::INTEGER:
            {
                if(!validate_INTEGER_token(key)){return;}
                int32_t string_to_int = stoi(key);                  //TODO: FIX FOR INT ENDINESS
                std::string converted(4, '\0');
                memcpy(&converted[0], &string_to_int, 4);
                file.insert_secondary_index<MyBtree4>(converted, record, *indexTree4, index_location, record_location, i);
                break;
            }
            case Type::CHAR32:
            {
                file.insert_secondary_index<MyBtree32>(strip_quotes(key), record, *indexTree32, index_location, record_location, i);
                break;
            }
            case Type::CHAR16:
            {
                file.insert_secondary_index<MyBtree16>(strip_quotes(key), record, *indexTree16, index_location, record_location, i);
                break;
            }
            case Type::CHAR8:
            {
                file.insert_secondary_index<MyBtree8>(strip_quotes(key), record, *indexTree8, index_location, record_location, i);
                break;
            }
        }
        
    }
    /*
        create table users (uid=int, username=string, password=int);
        insert into users (1, "smartpotato", 1234);
        find users where (username="smartpotato");
    */
    void execute(const std::string &input,File &file, 
    MyBtree32 *IndexTree32, 
    MyBtree8 *IndexTree8, 
    MyBtree4*IndexTree4,
    MyBtree16 *IndexTree16)
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
                if(tokens[1] == "table")    //handle creation of a table
                {
                    table_name = tokens[2];
                
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
                    file.insert_table<Node32, Node16, Node8, Node4>(&new_table); 

                    result.error = QueryError::None;
                    query_return(command,result); return;
                }
                else if(tokens[1] == "index")
                {
                    table_name = tokens[3];
                    std::string &column = tokens[4];
                    int column_index = 0;


                    for (auto &table_col: file.primary_table.columns)
                    {
                        if(table_col.name == column)
                        {
                            //found column
                            file.generate_index<MyBtree32, MyBtree16, MyBtree8, MyBtree4>
                            (column_index, *IndexTree32,*IndexTree16,*IndexTree8,*IndexTree4);

                            switch (file.primary_table.columns[0].type) //which primary type
                            {
                                case Type::INTEGER:{
                                    IndexTree4->tree_root = file.primary_table.columns[0].indexLocation;
                                    LeafNode4* leftmost = IndexTree4->find_leftmost_leaf();

                                    while (leftmost != nullptr)
                                    {
                                        for (int j = 0; j < leftmost->current_key_count; j++)
                                        {
                                            off_t record_location = leftmost->values[j];
                                            Record record = file.get_record(record_location);

                                            std::string key = record.get_token(column_index);

                                            Type secondaryKeyType = file.primary_table.columns[column_index].type;
                                            off_t index_location = file.primary_table.columns[column_index].indexLocation;

                                            insertSecondary(IndexTree32, IndexTree16, IndexTree8, IndexTree4, 
                                            record, key, file, record_location, column_index, index_location, secondaryKeyType);
                                        }
                                        if(leftmost->next_leaf != 0)
                                            leftmost = static_cast<LeafNode4*>(file.load_node<Node4>(leftmost->next_leaf));
                                        else
                                            leftmost = nullptr;
                                    }   
 
                                    break;
                                } 
                                case Type::CHAR32:{
                                    IndexTree32->tree_root = file.primary_table.columns[0].indexLocation;
                                    LeafNode32* leftmost = IndexTree32->find_leftmost_leaf();

                                    while (leftmost != nullptr)
                                    {
                                        for (int j = 0; j < leftmost->current_key_count; j++)
                                        {
                                            off_t record_location = leftmost->values[j];
                                            Record record = file.get_record(record_location);

                                            std::string key = record.get_token(column_index);

                                            Type secondaryKeyType = file.primary_table.columns[column_index].type;
                                            off_t index_location = file.primary_table.columns[column_index].indexLocation;

                                            insertSecondary(IndexTree32, IndexTree16, IndexTree8, IndexTree4, 
                                            record, key, file, record_location, column_index, index_location, secondaryKeyType);
                                        }
                                        if(leftmost->next_leaf != 0)
                                            leftmost = static_cast<LeafNode32*>(file.load_node<Node32>(leftmost->next_leaf));
                                        else
                                            leftmost = nullptr;
                                    }   
 
                                    break;
                                }
                                case Type::CHAR16:{
                                    IndexTree16->tree_root = file.primary_table.columns[0].indexLocation;
                                    LeafNode16* leftmost = IndexTree16->find_leftmost_leaf();

                                    while (leftmost != nullptr)
                                    {
                                        for (int j = 0; j < leftmost->current_key_count; j++)
                                        {
                                            off_t record_location = leftmost->values[j];
                                            Record record = file.get_record(record_location);

                                            std::string key = record.get_token(column_index);

                                            Type secondaryKeyType = file.primary_table.columns[column_index].type;
                                            off_t index_location = file.primary_table.columns[column_index].indexLocation;

                                            insertSecondary(IndexTree32, IndexTree16, IndexTree8, IndexTree4, 
                                            record, key, file, record_location, column_index, index_location, secondaryKeyType);
                                        }
                                        if(leftmost->next_leaf != 0)
                                            leftmost = static_cast<LeafNode16*>(file.load_node<Node16>(leftmost->next_leaf));
                                        else
                                            leftmost = nullptr;
                                    }   
 
                                    break;

                                }
                                case Type::CHAR8:{
                                    IndexTree8->tree_root = file.primary_table.columns[0].indexLocation;
                                    LeafNode8* leftmost = IndexTree8->find_leftmost_leaf();

                                    while (leftmost != nullptr)
                                    {
                                        for (int j = 0; j < leftmost->current_key_count; j++)
                                        {
                                            off_t record_location = leftmost->values[j];
                                            Record record = file.get_record(record_location);

                                            std::string key = record.get_token(column_index);

                                            Type secondaryKeyType = file.primary_table.columns[column_index].type;
                                            off_t index_location = file.primary_table.columns[column_index].indexLocation;

                                            insertSecondary(IndexTree32, IndexTree16, IndexTree8, IndexTree4, 
                                            record, key, file, record_location, column_index, index_location, secondaryKeyType);
                                        }
                                        if(leftmost->next_leaf != 0)
                                            leftmost = static_cast<LeafNode8*>(file.load_node<Node8>(leftmost->next_leaf));
                                        else
                                            leftmost = nullptr;
                                    }   
 
                                    break;
                                }
                            }

                            result.error = QueryError::None;
                            query_return(command,result); return;
                        }
                        column_index++;
                    }

                    catch_error(QueryError::SearchColumnNotFound);
                    return;
                    
                }
            }
            case Command::FIND: {
                if(tokens.size() != 4) { catch_error(QueryError::NotEnoughArgs); return;}
                table_name = tokens[1];
                if(!validate_table(table_name, file.primary_table))        { catch_error(QueryError::InvalidTable); return; }

                Args arg = arg_vec_split(tokens);
                std::string &search_column = arg[0].column;
                std::string search_word   = strip_quotes(arg[0].rhs);
                
                //if(!validatePrimaryFind(file.primary_table, search_column)) { catch_error(QueryError::SearchColumnNotFound);return;}
                Column *to_search = nullptr;

                for(auto &column : file.primary_table.columns)
                {
                    if(column.name == arg[0].column)
                    {
                        to_search = &column;
                    }
                }
                if(to_search == nullptr) { catch_error(QueryError::SearchColumnNotFound);return;}
                std::vector<Record> records;
                switch (to_search->type)
                {
                    case Type::INTEGER:
                    {
                        if(!validate_INTEGER_token(search_word)) return;
                        int val = stoi(search_word);
                        uint32_t be = htobe32(static_cast<uint32_t>(val));
                        std::string converted(4, '\0');
                        memcpy(&converted[0], &be, 4);
                        records = file.find<MyBtree4, Node4, InternalNode4, LeafNode4>(converted, *IndexTree4, to_search->indexLocation);
                        break;
                    }
                    case Type::CHAR32:
                    {
                        records = file.find<MyBtree32, Node32, InternalNode32, LeafNode32>(search_word, *IndexTree32, to_search->indexLocation);
                        break;
                    }
                    case Type::CHAR16:
                    {
                        records = file.find<MyBtree16, Node16, InternalNode16, LeafNode16>(search_word, *IndexTree16, to_search->indexLocation);
                        break;
                    }
                    case Type::CHAR8:
                    {
                        records = file.find<MyBtree8, Node8, InternalNode8, LeafNode8>(search_word, *IndexTree8, to_search->indexLocation);
                        break;
                    }

                }
                for (auto &record: records)
                {
                    std::cout << "Found: " << record.str << "\n";
                }
                std::cout << "Total Found" << records.size() << "\n";
                result.error = QueryError::None;
                query_return(command,result); return;
            }
            case Command::INSERT: {
                table_name = tokens[2];
                if(!validate_table(table_name, file.primary_table))        { catch_error(QueryError::InvalidTable); return; }
                if(tokens[3] != "from" && !validate_insert_length(tokens, file.primary_table))    { catch_error(QueryError::NotEnoughArgs); return; }

                if (tokens[3] == "from"){   //handle csv insertion //todo fix
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
                        off_t record_location;

                        if (record.length == -1) { catch_error(QueryError::ArgColumnMismatch); return;}
                        
                        switch (primaryKeyType){
                            case Type::INTEGER:
                                record_location = insertINTEGER(IndexTree4, record, insertLine[0], file);
                                break;
                            case Type::CHAR32:
                                record_location = insertCHAR32(IndexTree32, record, insertLine[0], file);
                                break;
                            case Type::CHAR16:
                                record_location = insertCHAR16(IndexTree16, record, insertLine[0], file);
                                break;
                            case Type::CHAR8:
                                record_location = insertCHAR8(IndexTree8, record, insertLine[0], file);
                                break;
                        }

                        if(file.primary_table.columns.size() != 1 && record_location != 0)
                        {
                            for (int i = 1; i < file.primary_table.columns.size(); i++)
                            {
                                Type secondaryKeyType = file.primary_table.columns[i].type;
                                off_t index_location = file.primary_table.columns[i].indexLocation;
                                std::string key = insertLine[i];
                                if(file.primary_table.columns[i].indexLocation != -1)
                                {
                                    insertSecondary(IndexTree32, IndexTree16, IndexTree8, IndexTree4, 
                                    record, key, file, record_location, i, index_location, secondaryKeyType);
                                }
                            }
                        }


                    }
                    

                    return;
                }
                else
                {                 
                    //handle standrd insertion
                    int primary_key_len = TypeUtil::type_len(file.primary_table.columns[0].type);
                    Type primaryKeyType = file.primary_table.columns[0].type;
                    const std::string &primary_key = strip_quotes(tokens[3]); 

                    Record record(tokens,file.primary_table, 3);

                    off_t record_location = 0;

                    if (record.length == -1) { catch_error(QueryError::ArgColumnMismatch); return;}

                    switch (primaryKeyType){
                        case Type::INTEGER:
                            record_location = insertINTEGER(IndexTree4, record, primary_key, file);
                            break;
                        case Type::CHAR32:
                            record_location = insertCHAR32(IndexTree32, record, primary_key, file);
                            break;
                        case Type::CHAR16:
                            record_location = insertCHAR16(IndexTree16, record, primary_key, file);
                            break;
                        case Type::CHAR8:
                            record_location = insertCHAR8(IndexTree8, record, primary_key, file);
                            break;
                        
                    }

                    if(file.primary_table.columns.size() != 1)
                    {
                        for (int i = 1; i < file.primary_table.columns.size(); i++)
                        {
                            Type secondaryKeyType = file.primary_table.columns[i].type;
                            off_t index_location = file.primary_table.columns[i].indexLocation;
                            std::string key = tokens[3+i];
                            if(file.primary_table.columns[i].indexLocation != -1)
                            {
                                insertSecondary(IndexTree32, IndexTree16, IndexTree8, IndexTree4, 
                                record, key, file, record_location, i, index_location, secondaryKeyType);
                            }
                        }
                    }
                    result.error = QueryError::None;
                    query_return(command,result); return;
                }
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