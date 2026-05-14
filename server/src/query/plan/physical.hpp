#pragma once
#include <memory>
#include <vector>
#include <string>
#include "../../config.h"
#include "../../relational/operations.hpp"
#include <variant>
#include "../../transactions/transaction.hpp"
#include "../../server/response.hpp"
#include "logical.hpp"

class Operator;
class Filter;
class Scan;
class Join;
class Session;

class PhysicalPlan
{
public:
    PhysicalPlan(Path &path, Database& database, Transaction* txn);

    void run_delete();
    void run_update(std::vector<AST::UpdateArg> &update_args);
    void run_select(const AST::SelectQuery* query, Session* session = nullptr);

    std::unique_ptr<Operator> root;
    std::vector<std::unique_ptr<Operator>> forest;

private:
    // ---- operator-tree construction ----
    bool check_if_indexed(const Predicate &predicate);
    bool check_if_filter_needed(const std::string &table_name);
    void build_predicate_buckets();
    void trim_scan_candidates();
    void build_forest();
    void compress_forest_into_root();
    void populate_filter(const std::string &table_name, Filter &filter);
    Predicate *pick_scan_predicate(const std::string &table_name);

    // ---- select execution helpers ----
    struct ColMapping
    {
        std::string table_name;
        size_t      col_idx;
        std::string header;
        int         display_width;
    };

    std::vector<ColMapping> build_col_map(const AST::SelectQuery* query) const;
    void run_aggregate(const AST::SelectQuery* query, Session* session);
    void run_mixed(const AST::SelectQuery* query, Session* session);
    void run_select_packets(const AST::SelectQuery* query,
                            const std::vector<ColMapping>& col_map,
                            Session* session);
    void run_select_stdout(const AST::SelectQuery* query,
                           const std::vector<ColMapping>& col_map);

    // ---- data ----
    Path path_;
    Database& database_;
    Transaction* txn;

    std::vector<const Predicate*> scan_candidates_;
    std::vector<const Predicate*> filter_candidates_;
    std::vector<const Predicate*> join_candidates_;
};
