#include "database.hpp"

Database::Database ()
{
    file = new File();
    file->database = this;

    std::vector<Table> fetched = file->load_table();

    for (auto table: fetched)
    {
        this->tableMap.insert({table.name, table});
        table.table_print();
    }


}
int Database::insert(const std::string& tableName, const StringVec& args)
{
    Record record(); 


    return 0;
}

void Database::flush()
{
    file->cache.flush_cache(); 
}