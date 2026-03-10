#pragma once
#include "../config.h"
#include "btree.hpp"
#include "../query/ast.hpp"
#include "../storage/record.hpp"
#include "../query/plan/planner.hpp"
#include "../query/validator.hpp"
#include "../query/plan/builder.hpp"
#include "../transactions/transaction.hpp"
#include <arpa/inet.h>
#include <mutex>
#include <variant>

class File;


class Database
{

public:
    File *file;

    //@maps table name to table object
    std::unordered_map<std::string,Table> tableMap;
    //@maps txn id to transaction object
    std::unordered_map<int, Transaction> transactions;


    // define index trees
    MyBtree32 index_tree32;
    MyBtree16 index_tree16;
    MyBtree8 index_tree8;
    MyBtree4 index_tree4;

    //@mutex for synchronizing access to transaction map and txn id generation
    std::mutex txn_mutex;
    //@atomic counter for generating unique transaction ids
    std::atomic<int> next_txn_id{0};


    Database ();


    void flush();

    int insert_table(Table& table, int txn_id);

    const Table& get_table(const std::string& tableName) const;
    Table& get_table(const std::string& tableName);

    void update_index_location(Table &table, int column_index, off_t new_index_location);
    void update_root_pointer(Table &table, off_t old_root, off_t new_root);
    int insert(const std::string& tableName, const StringVec& args, int txn_id);
    int erase(const std::string& tableName, Plan &plan, Transaction* txn);

    //@returns a new transaction id
    int create_transaction();
    int commit_transaction(int txn_id);

};
