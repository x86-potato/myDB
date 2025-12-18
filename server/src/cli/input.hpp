#pragma once

#include "../query/executor.hpp"
#include <string>
#include <vector>

class CLI {
public:
    explicit CLI(Executor& executor);
    void run();  // Starts the interactive loop

private:
    Executor& executor_;
};