#pragma once
#include <memory>
#include "../../config.h"
#include "../../relational/operations.hpp"
#include "planner.hpp"


class Pipeline
{
public:
    Pipeline(Path &path, Database& database);

    void Execute();

    std::unique_ptr<Operator> root;

    std::vector<std::unique_ptr<Operator>> forest;

private:
    Path path_; //work on a copy
    Database& database_;


    bool check_if_indexed(const Predicate &predicate);
    bool check_if_filter_needed(const std::string &table_name);

    void build_buckets();
    void sort_buckets();
    void print_buckets();

    void populate_filter(const std::string &table_name, Filter &Filter);
    void build_forest();
    void compress_forest();


    std::vector<const Predicate*> scan_candidates_;
    std::vector<const Predicate*> filter_candidates_;
    std::vector<const Predicate*> join_candidates_;

    Predicate *pick_scan_predicate(const std::string &table_name);

};