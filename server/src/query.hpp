#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "config.h"
#include "table.hpp"
#include "tokenizer.hpp"
#include "file.hpp"
#include "btree.hpp"
#include "validator.hpp"




struct arg {
    std::string column;
    std::string rhs;
    arg(std::string column, std::string rhs);
};

using Args = std::vector<arg>;

enum class Command {
    None,
    CREATE,
    INSERT,
    INDEX,
    FIND
};

namespace CommandUtil {
    Command string_to_command(std::string input);
}

enum class QueryError {
    None,
    NoCommand,
    NotEnoughArgs,
    ArgColumnMismatch,
    NoTable,
    InvalidTable,
    NoArgs,
    InvalidType,
    ColumnAlreadyExists,
    SearchColumnNotFound,
    InsertFromFailed
};

std::string errorToString(QueryError e);

struct QueryResult {
    QueryError error = QueryError::None;
    std::string message;

    QueryResult() = default;
    QueryResult(QueryError error, std::string message);
};

namespace Query {

    void query_return(Command command, QueryResult result);

    Args arg_vec_split(const StringVec& tokens);

    void execute(
        const std::string& input,
        File& file,
        MyBtree32* IndexTree32,
        MyBtree8* IndexTree8,
        MyBtree4* IndexTree4
    );

} // namespace Query