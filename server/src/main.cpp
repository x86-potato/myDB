#include "config.h"
#include "core/database.hpp"
#include "core/btree.hpp"
#include "storage/file.hpp"
//#include "query/querylegacy.hpp"
#include "storage/record.hpp"


#include "query/parse/lexer.hpp"

#include "query/execute/executor.hpp"

#include "cli/input.hpp"

#include "server/server.hpp"

#include <iostream>
#include <iomanip>
#include <cstring>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>




int main()
{
    // Ignore SIGPIPE so that writing to a disconnected TCP client
    // returns EPIPE (checked via write's return value) instead of
    // killing the entire server process.
    signal(SIGPIPE, SIG_IGN);

    Database database;
    Executor executor(database);
    CLI cli(executor);
    Server server(database, executor);
    LockManager lock_manager(database.file->cache);
    database.file->lock_manager = &lock_manager;

    // std::thread bench_thread([&]() {
    //     std::this_thread::sleep_for(std::chrono::milliseconds(200)); // let server settle
    //     run_insert_bench(executor, database, 1'000'000);
    // });

    // std::thread twin_thread([&]() {
    //     std::this_thread::sleep_for(std::chrono::milliseconds(200));
    //     run_triplet_bench(executor, 300'000);
    // });

    server.start();

    // twin_thread.join();

    //cli.run();          // Starts the interactive CLI loop

    database.flush();   // Save any pending data on exit

    return 0;
}
