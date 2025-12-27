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

};