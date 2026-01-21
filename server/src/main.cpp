#include "config.h"
#include "core/database.hpp" 
#include "core/btree.hpp"
#include "storage/file.hpp"
//#include "query/querylegacy.hpp"
#include "storage/record.hpp"


#include "query/lexer.hpp"  

#include "query/executor.hpp"

#include "cli/input.hpp"

#include <iostream>
#include <cstring>


#include <chrono>
#include <thread>








int main()
{
    Database database;
    Executor executor(database);
    CLI cli(executor);

    std::thread wal_worker(&Cache::WAL, std::ref(database.file->cache)); // Start WAL thread

    wal_worker.detach(); // Detach the thread to run independently

    cli.run();          // Starts the interactive CLI loop

    database.flush();   // Save any pending data on exit

    wal_worker.join(); // Ensure WAL thread has finished before exiting
    return 0;
}

