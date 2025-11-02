#include "validator.hpp"
#include <cstddef>

bool validate_table(const std::string& table_name, const Table& loaded_table)
{
    if (loaded_table.name != table_name) return false;
    return true;
}

bool validate_type(Type type)
{
    if (type == Type::UNKNOWN) return false;
    return true;
}

bool validate_insert_length(const StringVec& tokens, const Table& table)
{
    if (tokens.size() - 3 != table.columns.size()) return false;
    return true;
}

bool validate_INTEGER_token(const std::string& s)
{
    if (s.empty()) return false;

    // Check that all characters are digits
    for (char c : s) {
        if (c < '0' || c > '9') return false;
    }

    const char INT_MAX_STR[] = "2147483647";
    size_t len = s.size();

    if (len < 10) {
        return true; // definitely fits in 32-bit signed int (positive side)
    } else if (len > 10) {
        return false; // too large
    } else {
        // same length as INT_MAX, compare lexicographically
        for (size_t i = 0; i < len; ++i) {
            if (s[i] < INT_MAX_STR[i]) return true;  // smaller than max
            if (s[i] > INT_MAX_STR[i]) return false; // larger than max
        }
        return true; // exactly equal to 2147483647
    }
}