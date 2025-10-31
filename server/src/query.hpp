#pragma once

#include <iostream>
#include <vector>
#include <algorithm>

#include "table.hpp"
#include "lexer.hpp"
#include "file.hpp"
#include "btree.hpp"


using BtreePlus32 = BtreePlus<Node32, LeafNode32, InternalNode32>;
using BtreePlus8  = BtreePlus<Node8, LeafNode8,InternalNode8>;






enum class Command
{
    None,
    CREATE,
    INSERT,
    INDEX,
    FIND,
    MODIFY

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
        if (input == "MODIFY")  return Command::MODIFY;

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
    Success,
    NoCommand,
    NoTable,
    NoArgs,
    InvalidType,
    ColumnAlreadyExists
    
};

struct QueryResult {
    QueryError error;
    std::string message;

    QueryResult(QueryError error, std::string message) : error(error), message(message)
    {
        
    }   
};


class Query
{


public:
    //common
    Command command = Command::None;
    std::string table;
    std::vector<arg> args;

    BtreePlus32 *IndexTree32 = nullptr;
    BtreePlus8 *IndexTree8 = nullptr;



    Query(std::string input, BtreePlus32 *IndexTree32, BtreePlus8 *IndexTree8) 
    : IndexTree32(IndexTree32), IndexTree8(IndexTree8)
    {
        Lexer lexer; 
        Lexer_Output lexer_out;
        lexer_out = lexer.lexer(input);

        command = CommandUtil::string_to_command(lexer_out.command);
        table   = lexer_out.table;


        arg_vec_split(lexer_out.arg_strings);



    }

    

    void arg_vec_split(std::vector<std::string> &to_split)
    {
        std::vector<arg> output;
        for (auto &str: to_split)
        {
            
            int split_index = str.find('=');
            if(split_index != std::string::npos)
            {
                std::string arg1 = std::string(str.begin(),str.begin() + split_index);
                std::string arg2 = std::string(str.begin() + split_index + 1, str.end());


                arg temp_arg = arg(arg1, arg2);
                
                args.push_back(temp_arg);
            }

        }

    }


    QueryResult execute(File &file)
    {
        if(command == Command::None) return QueryResult{QueryError::NoCommand,""};  
        if(table.length() == 0)      return QueryResult{QueryError::NoTable,""};  
        if(args.size() == 0)         return QueryResult{QueryError::NoArgs,""};  
    
        switch (command)
        {
            case Command::CREATE:
            {
                Table new_table;



                return QueryResult{create(new_table, file), ""};
                
            }
            case Command::FIND:
            {
                return QueryResult{find(file), ""};
                break;
            }
            case Command::INDEX:
            {

                break;
            }
            case Command::INSERT:
            {
                return QueryResult{insert(file.primary_table, file), ""};
                break;
            }
            case Command::MODIFY:
            {
                break;

            }
            case Command::None:
            {
                break;

            }
        }
        return QueryResult{QueryError::None,""};  

    }

    QueryError find(File &file)
    {
        //for now handle a single search query
        const std::string* query = &args[0].rhs;

        if(file.primary_table.columns[0].type == Type::STRING)
        {
            file.find<MyBtree32, Node32, InternalNode32, LeafNode32>(*query, *IndexTree32);
        }

        return QueryError::Success;
    }

    QueryError create(Table &new_table, File &file)
    {
        new_table.table_name = table;
        for (auto &arg : args)
        {
            Type type = TypeUtil::string_to_type(arg.rhs);


            if(type == Type::UNKNOWN) return QueryError::InvalidType; //if type is not known

            Column new_column(arg.column,type);

            new_table.columns.push_back(new_column);
        }

        file.insert_table(&new_table); 

        return QueryError::Success;
    }

    QueryError insert(Table &given_table, File &file)
    {
        int key_len = TypeUtil::type_len(file.primary_table.columns[0].type);
        if(key_len == 8 && args[0].rhs.length() <= 8) 
        {
            
            Record record(*this,file.primary_table);

            //std::cout << "\n inserting record: " << record.str;

            file.insert_data<MyBtree8,Node8,LeafNode8,InternalNode8>
            (args[0].rhs,record,*IndexTree8);
        }
        else if(key_len == 32 && args[0].rhs.length() <= 32)
        {
            Record record(*this,file.primary_table);

            //std::cout << "\n inserting record: " << record.str;

            file.insert_data<MyBtree32,Node32,LeafNode32,InternalNode32>
            (args[0].rhs,record,*IndexTree32);    
            
        }
    }
    QueryError remove(Table &given_table, File &file)
    {
        
    }


    std::string errorToString(QueryError e) 
    {
        switch(e) 
        {
            case QueryError::None:          return "Success";
            case QueryError::NoCommand:     return "No command given";
            case QueryError::NoTable:       return "Table name is missing";
            case QueryError::NoArgs:        return "No columns specified";
            case QueryError::InvalidType:   return "Invalid column type";
            default:                        return "Unknown error";
        }
    }
};

class Find_Query : public Query
{
public:
    std::vector<Operator> operators;
    std::vector<arg> output_args;
};

class Modify : public Query
{
public:
    std::vector<arg> modify_args;
};

