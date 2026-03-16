#include "config.h"
#include "core/database.hpp"
#include "core/btree.hpp"
#include "storage/file.hpp"
//#include "query/querylegacy.hpp"
#include "storage/record.hpp"


#include "query/lexer.hpp"

#include "query/executor.hpp"

#include "cli/input.hpp"

#include "server/server.hpp"

#include <iostream>
#include <cstring>


#include <chrono>
#include <thread>








int main()
{
    Database database;
    Executor executor(database);
    CLI cli(executor);
    Server server(database, executor);
    LockManager lock_manager(database.file->cache);
    database.file->lock_manager = &lock_manager;

    server.start();



    //cli.run();          // Starts the interactive CLI loop

    database.flush();   // Save any pending data on exit

    return 0;
}
