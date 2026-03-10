#pragma once

#include "../query/executor.hpp"
#include "../server/server.hpp"
#include <string>
#include <vector>
#include <thread>

class Executor;
class Server;

class CLI {
public:
    CLI(Executor& executor);
    void run(Server& server);  // Starts the interactive loop

private:
    Executor& executor_;
};