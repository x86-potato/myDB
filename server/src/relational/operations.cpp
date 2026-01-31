#include "operations.hpp"

void Operator::set_key_on_column(const Key& key, const std::string& column_name)
{
    (void)column_name;
    set_key(key); // Default implementation
}

void Operator::update_last_deleted_key(const Key& key)
{
    last_deleted_key = key;
}