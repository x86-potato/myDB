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




using BtreePlus32 = BtreePlus<Node32, LeafNode32, InternalNode32>;
using BtreePlus8  = BtreePlus<Node8,  LeafNode8,  InternalNode8>;

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
    ColumnAlreadyExists
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
        BtreePlus32* IndexTree32,
        BtreePlus8* IndexTree8
    );

} // namespace Query