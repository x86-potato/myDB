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

private:
    Path path_; //work on a copy


    bool check_if_indexed(const Predicate &predicate);
    void build_buckets();
    void sort_buckets();
    void print_buckets();


    std::vector<const Predicate*> scan_candidates_;
    std::vector<const Predicate*> filter_candidates_;
    std::vector<const Predicate*> join_candidates_;

    const Database& database_;
};