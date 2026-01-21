#include "operations.hpp"

void Operator::set_key_on_column(const std::optional<Key>& key, const std::string& column_name)
{
    (void)column_name;
    set_key(key); // Default implementation
}
