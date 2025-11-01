#ifndef VALIDATION_HPP
#define VALIDATION_HPP

#include <string>
#include <vector>
#include "config.h"
#include "table.hpp"

// Type alias for clarity
using StringVec = std::vector<std::string>;

/**
 * @brief Validates that the loaded table's name matches the expected table name.
 * @param table_name Expected name of the table.
 * @param loaded_table The table object loaded from storage.
 * @return true if names match, false otherwise.
 */
bool validate_table(const std::string& table_name, const Table& loaded_table);

/**
 * @brief Validates that the given type is not UNKNOWN.
 * @param type The Type enum value to validate.
 * @return true if type is valid (not UNKNOWN), false otherwise.
 */
bool validate_type(Type type);

/**
 * @brief Checks if the number of tokens in an INSERT statement matches the table's column count.
 *        Assumes tokens[0] = "INSERT", tokens[1] = "INTO", tokens[2] = table_name, then values follow.
 * @param tokens Vector of parsed SQL tokens.
 * @param table The table schema.
 * @return true if (tokens.size() - 3) == number of columns, false otherwise.
 */
bool validate_insert_length(const StringVec& tokens, const Table& table);

/**
 * @brief Validates that a string represents a valid 32-bit signed integer (INT) in range [-2147483648, 2147483647].
 *        This function only checks positive range since negative sign is handled externally.
 * @param s The string to validate (assumed non-negative or without '-').
 * @return true if the string is a valid integer within INT_MAX, false otherwise.
 */
bool validate_INTEGER_token(const std::string& s);

#endif // VALIDATION_HPP