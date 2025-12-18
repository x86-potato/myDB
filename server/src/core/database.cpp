#include "database.hpp"

Database::Database ()
{
    file = new File();
    file->database = this;

    index_tree32.file = file;  
    index_tree16.file = file;  
    index_tree8.file = file;
    index_tree4.file = file;

    std::vector<Table> fetched = file->load_table();

    for (auto table: fetched)
    {
        this->tableMap.insert({table.name, table});
        table.table_print();
    }


}
int Database::insert(const std::string& tableName, const StringVec& args)
{

    file->cache.read_block(0);
    Table &table = tableMap.at(tableName);
    Record record(args, table); 

    std::string key;
    off_t insertion_result = 0;

    switch (table.columns[0].type)
    {
        case Type::INTEGER:
        {
            int32_t number = std::stoi(args[0]);

            int8_t casted_int[4];

            memcpy(casted_int, &number, 4);

            for (int j = 0; j < 4; j++)
            {
                key += static_cast<char>(casted_int[j]);
            }

            insertion_result = file->insert_primary_index<MyBtree4>(key,record, index_tree4, table);
            break;
        }
        case Type::CHAR8:   
            key = args[0]; 
            insertion_result = file->insert_primary_index<MyBtree8>(key,record, index_tree8, table);
            break;
        case Type::CHAR16:  
            key = args[0]; 
            insertion_result = file->insert_primary_index<MyBtree16>(key,record, index_tree16, table);
            break;
        case Type::CHAR32:  
            key = args[0]; 
            insertion_result = file->insert_primary_index<MyBtree32>(key,record, index_tree32, table);
            break;
        default:
            std::cout << "Error: Column type not recognized";
            return 1;
        
    }

    if(insertion_result == -1) 
    {
        std::cout << "Insertion Error key: " << key << " is already in the index tree";
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
                    file->insert_secondary_index<MyBtree4>(secondary_key, table, 
                        index_tree4,insertion_result, column_index);
                    break;
                case Type::CHAR8:
                    file->insert_secondary_index<MyBtree8>(secondary_key, table, 
                        index_tree8, insertion_result, column_index);
                    break;
                case Type::CHAR16:
                    file->insert_secondary_index<MyBtree16>(secondary_key, table, 
                        index_tree16, insertion_result, column_index);
                    break;
                case Type::CHAR32:
                    file->insert_secondary_index<MyBtree32>(secondary_key, table, 
                        index_tree32, insertion_result, column_index);
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

int Database::select(const std::string& tableName, const StringVec& args)
{
    Table &table = tableMap.at(tableName);

}




void Database::update_index_location(Table &table, int column_index, off_t new_index_location)
{
    file->update_table_index_location(table, column_index, new_index_location);
}



void Database::flush()
{
    file->cache.flush_cache(); 
}