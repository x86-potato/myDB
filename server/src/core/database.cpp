#include "database.hpp"

Database::Database ()
{


    file = new File();
    file->database = this;

    index_tree32.file = file;
    index_tree16.file = file;
    index_tree8.file = file;
    index_tree4.file = file;

    std::vector<Table> fetched = file->load_tables();

    for (auto table: fetched)
    {
        this->tableMap.insert({table.name, table});
        table.table_print();
    }

    wal = new WAL(file);

    wal->recover(*this);

}

int Database::insert_table(Table& table, int txn_id)
{
    if (table.columns.size() == 0)
    {
        std::cout << "Error: Cannot create table with no columns." << std::endl;
        return 1;
    }

    off_t result = file->insert_table<Node32, Node16, Node8, Node4>(table, txn_id);
    tableMap.insert({table.name, table});
    return 0;
}

const Table& Database::get_table(const std::string& tableName) const
{
    return tableMap.at(tableName);
}

Table& Database::get_table(const std::string& tableName)
{
    return tableMap.at(tableName);
}

int Database::insert(const std::string& tableName, const StringVec& args, int txn_id)
{

    Table &table = tableMap.at(tableName);
    Record record(args, table);


    std::string key;
    off_t insertion_result = 0;

    auto it = transactions.find(txn_id);
    if (it == transactions.end()) {
        std::cout << "Error: No active transaction for insert operation." << std::endl;
        return 1;
    }
    Transaction& txn = it->second;

    switch (table.columns[0].type)
    {
        case Type::INTEGER:
        {
            int32_t number = std::stoi(args[0]);
            uint32_t big_endian = htonl(static_cast<uint32_t>(number));

            key.append(reinterpret_cast<const char*>(&big_endian), sizeof(big_endian));


            insertion_result = file->insert_primary_index<MyBtree4>(key,record, index_tree4, table, txn_id);
            break;
        }
        case Type::CHAR8:
            key = args[0];
            insertion_result = file->insert_primary_index<MyBtree8>(strip_quotes(key),record, index_tree8, table, txn_id);
            break;
        case Type::CHAR16:
            key = args[0];
            insertion_result = file->insert_primary_index<MyBtree16>(strip_quotes(key),record, index_tree16, table, txn_id);
            break;
        case Type::CHAR32:
            key = args[0];
            insertion_result = file->insert_primary_index<MyBtree32>(strip_quotes(key),record, index_tree32, table, txn_id);
            break;
        default:
            std::cout << "Error: Column type not recognized";
            return 1;

    }

    if(insertion_result == -1)
    {
        if(table.columns[0].type == Type::INTEGER)
        {
            int32_t number = std::stoi(args[0]);

            std::cout << "Insertion Error key: " << number << " is already in the index tree\n";
        }
        else
        {
            std::cout << "Insertion Error key: " << key << " is already in the index tree\n";
        }
        return 1;
    }

    int column_index = 0;
    for (auto &column : table.columns)
    {
        if (column.indexLocation != -1 && &column != &table.columns[0])
        {
            std::string secondary_key = args[column_index];

            switch (column.type)
            {
                case Type::INTEGER:
                {
                    int32_t number = std::stoi(secondary_key);
                    uint32_t big_endian = htonl(static_cast<uint32_t>(number));

                    std::string int_key;
                    int_key.append(reinterpret_cast<const char*>(&big_endian), sizeof(big_endian));

                    file->insert_secondary_index<MyBtree4>(int_key, table,
                        index_tree4,insertion_result, column_index, txn_id);
                    break;
                }
                case Type::CHAR8:
                    file->insert_secondary_index<MyBtree8>(strip_quotes(secondary_key), table,
                        index_tree8, insertion_result, column_index, txn_id);
                    break;
                case Type::CHAR16:
                    file->insert_secondary_index<MyBtree16>(strip_quotes(secondary_key), table,
                        index_tree16, insertion_result, column_index, txn_id);
                    break;
                case Type::CHAR32:
                    file->insert_secondary_index<MyBtree32>(strip_quotes(secondary_key), table,
                        index_tree32, insertion_result, column_index, txn_id);
                    break;
                default:
                    std::cout << "Error: Column type not recognized";
                    return 1;
            }
        }
        column_index++;
    }


    return 0;
}

int Database::erase(const std::string& tableName, Plan& plan, Transaction* txn)
{

    if (plan.paths.size() == 0) return 1;

    if (plan.paths.size() == 1)
    {
        Pipeline plan_executor(plan.paths[0], *this, txn);

        plan_executor.ExecuteDelete();
        return 0;
    }

    return 1;
}


void Database::update_index_location(Table &table, int column_index, off_t new_index_location)
{
    file->update_table_index_location(table, column_index, new_index_location);
}

void Database::update_root_pointer(Table &table, off_t old_root, off_t new_root)
{
    file->update_root_pointer(&table, old_root, new_root);
}

void Database::flush()
{
    file->cache.flush_cache();
}

int Database::create_transaction()
{
    std::lock_guard<std::mutex> lock(txn_mutex);

    int txn_id = next_txn_id.fetch_add(1);
    //this line caused issue due to refrence member move assignemnt being deleted
    //transactions.insert({txn_id, Transaction(txn_id, file->cache, *file->lock_manager)});

    transactions.try_emplace(txn_id, txn_id, file->cache, *file->lock_manager, *wal);



    transactions.at(txn_id).begin();
    return txn_id;
}


int Database::commit_transaction(int txn_id)
{
    std::lock_guard<std::mutex> lock(txn_mutex);
    auto find = transactions.find(txn_id);
    if(find == transactions.end())
    {
        std::cout << "Error: Transaction ID " << txn_id << " not found." << std::endl;
        return -1;
    }
    transactions.at(txn_id).commit();

    transactions.erase(txn_id);
    std::cout << "commited";


    return 0;
}
