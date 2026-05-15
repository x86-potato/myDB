#include "executor.hpp"

void Executor::execute_insert(AST::InsertQuery* query, Session& session)
{
    int txn_id = session.current_transaction_id;
    if (txn_id == -1)
    {
        session.send_error("Error: No active transaction for insert operation.");
        return;
    }

    if (!validateInsertQuery(*query, database))
        return;

    // --- collect values ---
    StringVec values;
    for (auto& arg : query->args)
        values.push_back(arg.value);

    Table& table = database.tableMap.at(query->tableName);
    Record record(values, table);

    // --- acquire transaction ---
    Transaction* txn_ptr = nullptr;
    {
        std::lock_guard<std::mutex> lock(database.txn_mutex);
        auto it = database.transactions.find(txn_id);
        if (it == database.transactions.end())
        {
            session.send_error("Error: No active transaction for insert operation.");
            return;
        }
        txn_ptr = &it->second;
    }
    Transaction& txn = *txn_ptr;

    // --- encode primary key and insert into primary index ---
    std::string key;
    off_t record_location = 0;

    switch (table.columns[0].type)
    {
        case Type::INTEGER:
        {
            int32_t number = std::stoi(values[0]);
            uint32_t big_endian = htonl(static_cast<uint32_t>(number));
            key.append(reinterpret_cast<const char*>(&big_endian), sizeof(big_endian));
            record_location = database.file->insert_primary_index<MyBtree4>(
                key, record, database.index_tree4, table, txn);
            break;
        }
        case Type::CHAR8:
            key = values[0];
            record_location = database.file->insert_primary_index<MyBtree8>(
                strip_quotes(key), record, database.index_tree8, table, txn);
            break;
        case Type::CHAR16:
            key = values[0];
            record_location = database.file->insert_primary_index<MyBtree16>(
                strip_quotes(key), record, database.index_tree16, table, txn);
            break;
        case Type::CHAR32:
            key = values[0];
            record_location = database.file->insert_primary_index<MyBtree32>(
                strip_quotes(key), record, database.index_tree32, table, txn);
            break;
        default:
            session.send_error("Error: Column type not recognized");
            return;
    }

    if (record_location == -1)
    {
        std::string duplicate_key = (table.columns[0].type == Type::INTEGER)
            ? std::to_string(std::stoi(values[0]))
            : key;
        session.send_error("Insert failed: duplicate primary key " + duplicate_key);
        return;
    }

    // --- insert into any secondary indexes ---
    for (int col_idx = 0; col_idx < static_cast<int>(table.columns.size()); col_idx++)
    {
        const Column& col = table.columns[col_idx];
        if (col.indexLocation == -1 || col_idx == 0)
            continue;

        std::string skey = values[col_idx];

        switch (col.type)
        {
            case Type::INTEGER:
            {
                int32_t number = std::stoi(skey);
                uint32_t big_endian = htonl(static_cast<uint32_t>(number));
                std::string int_key;
                int_key.append(reinterpret_cast<const char*>(&big_endian), sizeof(big_endian));
                database.file->insert_secondary_index<MyBtree4>(
                    int_key, table, database.index_tree4, record_location, col_idx, txn_id);
                break;
            }
            case Type::CHAR8:
                database.file->insert_secondary_index<MyBtree8>(
                    strip_quotes(skey), table, database.index_tree8, record_location, col_idx, txn_id);
                break;
            case Type::CHAR16:
                database.file->insert_secondary_index<MyBtree16>(
                    strip_quotes(skey), table, database.index_tree16, record_location, col_idx, txn_id);
                break;
            case Type::CHAR32:
                database.file->insert_secondary_index<MyBtree32>(
                    strip_quotes(skey), table, database.index_tree32, record_location, col_idx, txn_id);
                break;
            default:
                session.send_error("Error: Column type not recognized");
                return;
        }
    }

    session.send_ok();
}
