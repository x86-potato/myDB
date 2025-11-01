#pragma once

#include <iostream>
#include <vector>
#include <algorithm>

#include "config.h"
#include "table.hpp"
#include "tokenizer.hpp"
#include "file.hpp"
#include "btree.hpp"
#include "validator.hpp"


using BtreePlus32 = BtreePlus<Node32, LeafNode32, InternalNode32>;
using BtreePlus8  = BtreePlus<Node8, LeafNode8,InternalNode8>;


struct arg {
    std::string column;
    std::string rhs;
    arg(std::string column, std::string rhs);
};

using Args = std::vector<arg>;


enum class Command
{
    None,
    CREATE,
    INSERT,
    INDEX,
    FIND

};

namespace CommandUtil
{
    inline Command string_to_command(std::string input)
    {
        std::transform(input.begin(), input.end(), input.begin(), ::toupper);
        if (input == "CREATE")  return Command::CREATE;
        if (input == "INSERT")  return Command::INSERT;
        if (input == "INDEX")   return Command::INDEX;
        if (input == "FIND")    return Command::FIND;


        return Command::None; 
    }
};

enum class Operator
{
    AND,
    OR,
    NOT
};



enum class QueryError {
    None,
    NoCommand,
    NotEnoughArgs,
    ArgColumnMismatch,
    NoTable,
    InvalidTable,
    NoArgs,
    InvalidType,
    ColumnAlreadyExists
    
};
inline std::string errorToString(QueryError e) 
{
    switch(e) 
    {
        case QueryError::None:                      return "Success";
        case QueryError::NoCommand:                 return "No command given";
        case QueryError::NotEnoughArgs:             return "Too few arguments";
        case QueryError::ArgColumnMismatch:         return "Provided argument doesnt match column";
        case QueryError::NoTable:                   return "Table name is missing";
        case QueryError::NoArgs:                    return "No columns specified";
        case QueryError::InvalidTable:              return "Table does not exist";                       
        case QueryError::InvalidType:               return "Invalid column type";                       
        case QueryError::ColumnAlreadyExists:       return "Column already created"; 
        
    }
    return "Unknown error";
}

struct QueryResult {
    QueryError error;
    std::string message;

    QueryResult(QueryError error, std::string message) : error(error), message(message)
    {
        
    }   
    QueryResult(){}
};



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
    /*
        create users (uid=int, username=string, password=int);
        insert into users (1, "smartpotato", 1234);
        find users where (username="smartpotato");
    */
    void execute(const std::string &input,File &file, BtreePlus32 *IndexTree32, BtreePlus8 *IndexTree8)
    {
        StringVec tokens;
        tokenize(input, tokens);

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
                validate_table(table_name, file.primary_table);

                break;
            }
            case Command::INSERT: {
                table_name = tokens[2];
                if(!validate_table(table_name, file.primary_table))        { catch_error(QueryError::InvalidTable); return; }
                if(!validate_insert_length(tokens, file.primary_table))    { catch_error(QueryError::NotEnoughArgs); return; }


                int key_len = TypeUtil::type_len(file.primary_table.columns[0].type);
                const std::string &primary_key = tokens[3]; 
                //if(key_len == 8 && tokens[3].length() <= 8) 
                //{
                //     Record record(*this,file.primary_table);


                //     file.insert_data<MyBtree8,Node8,LeafNode8,InternalNode8>(tokens[i],record,*IndexTree8);
                // }
                if(key_len == 32 && primary_key.length() <= 32)
                {
                    Record record(tokens,file.primary_table);
                    if (record.length == -1)
                    {
                        catch_error(QueryError::ArgColumnMismatch); 
                        return;
                    }
                    else
                    {
                        std::cout << record.str;
                        file.insert_data<MyBtree32,Node32,LeafNode32,InternalNode32>(primary_key,record,*IndexTree32);    
                    }
                }

                

                break;
            }
            case Command::None: {
                result.error = QueryError::NoCommand;
                break;
            }
        }



        //arg_vec_split(lexer_out.arg_strings);


       // std::cout << "Query: " << errorToString(execute(file).error);

    }









}

