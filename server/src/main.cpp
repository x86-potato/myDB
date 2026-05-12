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
#include <iomanip>
#include <cstring>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>

static void run_insert_bench(Executor& executor, Database& database, int n)
{
    Session fake;
    fake.session_id = 1;
    fake.client_fd  = -1;

    unsigned int seed = 42;
    auto next_uid = [&]() -> unsigned int {
        seed = seed * 1664525u + 1013904223u; // LCG
        return seed % 1'999'000'000u + 1u;
    };

    auto t0 = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < n; i++)
    {
        unsigned int uid = next_uid();
        std::string q = "insert into users (" + std::to_string(uid) + ", \"u" + std::to_string(uid) + "\");";
        executor.execute(q, fake);
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double sec = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "[bench] " << n << " inserts in " << sec << "s  ("
              << (int)(n / sec) << " QPS)\n";
}

static void run_twin_bench(Executor& executor, int inserts_per_table)
{
    std::atomic<int> users_done{0};
    std::atomic<int> products_done{0};
    std::atomic<bool> stop{false};

    auto user_worker = [&]() {
        Session s;
        s.session_id = 1;
        s.client_fd  = -1;
        unsigned int seed = static_cast<unsigned int>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());
        while (!stop) {
            seed = seed * 1664525u + 1013904223u;
            unsigned int uid = seed % 1'999'000'000u + 1u;
            std::string q = "insert into users (" + std::to_string(uid)
                          + ", \"u" + std::to_string(uid) + "\");";
            executor.execute(q, s);
            int v = ++users_done;
            if (v >= inserts_per_table) stop = true;
        }
    };

    auto product_worker = [&]() {
        Session s;
        s.session_id = 2;
        s.client_fd  = -1;
        unsigned int seed = static_cast<unsigned int>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()) + 1000000u;
        while (!stop) {
            seed = seed * 1664525u + 1013904223u;
            unsigned int pid = seed % 1'999'000'000u + 1u;
            unsigned int qty = pid % 9999 + 1;
            std::string name = "prod" + std::to_string(pid);
            if (name.size() > 32) name.resize(32);
            std::string q = "insert into products (" + std::to_string(pid)
                          + ", \"" + name + "\", " + std::to_string(qty) + ");";
            executor.execute(q, s);
            int v = ++products_done;
            if (v >= inserts_per_table) stop = true;
        }
    };

    // Print header
    std::cout << std::right
              << std::setw(6)  << "Time" << " | "
              << std::setw(10) << "Users" << " | "
              << std::setw(8)  << "U-QPS" << " | "
              << std::setw(10) << "Products" << " | "
              << std::setw(8)  << "P-QPS" << " | "
              << std::setw(10) << "Total" << " | "
              << std::setw(8)  << "Avg QPS" << "\n";
    std::cout << std::string(80, '-') << "\n";

    auto t0 = std::chrono::steady_clock::now();
    int last_u = 0, last_p = 0;
    auto last_ts = t0;

    std::thread ut(user_worker);
    std::thread pt(product_worker);

    while (!stop) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto now  = std::chrono::steady_clock::now();
        double elapsed  = std::chrono::duration<double>(now - t0).count();
        double interval = std::chrono::duration<double>(now - last_ts).count();

        int u = users_done.load();
        int p = products_done.load();
        int u_qps = (int)((u - last_u) / interval);
        int p_qps = (int)((p - last_p) / interval);
        int avg_qps = elapsed > 0 ? (int)((u + p) / elapsed) : 0;

        std::cout << std::fixed << std::setprecision(1)
                  << std::setw(6)  << elapsed << "s | "
                  << std::setw(10) << u << " | "
                  << std::setw(8)  << u_qps << " | "
                  << std::setw(10) << p << " | "
                  << std::setw(8)  << p_qps << " | "
                  << std::setw(10) << (u + p) << " | "
                  << std::setw(8)  << avg_qps << "\n";

        last_u  = u;
        last_p  = p;
        last_ts = now;
    }

    ut.join();
    pt.join();

    double total = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::cout << "\n[twin-bench] users=" << users_done
              << " products=" << products_done
              << " total=" << (users_done + products_done)
              << " time=" << std::fixed << std::setprecision(2) << total << "s"
              << " avg_qps=" << (int)((users_done + products_done) / total) << "\n";
}

static void run_triplet_bench(Executor& executor, int inserts_per_table)
{
    std::atomic<int> users_done{0};
    std::atomic<int> products_done{0};
    std::atomic<int> messages_done{0};
    std::atomic<bool> stop{false};

    auto user_worker = [&]() {
        Session s;
        s.session_id = 1;
        s.client_fd  = -1;
        unsigned int seed = static_cast<unsigned int>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());
        while (!stop) {
            seed = seed * 1664525u + 1013904223u;
            unsigned int uid = seed % 1'999'000'000u + 1u;
            std::string q = "insert into users (" + std::to_string(uid)
                          + ", \"u" + std::to_string(uid) + "\");";
            executor.execute(q, s);
            if (++users_done >= inserts_per_table) stop = true;
        }
    };

    auto product_worker = [&]() {
        Session s;
        s.session_id = 2;
        s.client_fd  = -1;
        unsigned int seed = static_cast<unsigned int>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()) + 1000000u;
        while (!stop) {
            seed = seed * 1664525u + 1013904223u;
            unsigned int pid = seed % 1'999'000'000u + 1u;
            unsigned int qty = pid % 9999 + 1;
            std::string name = "prod" + std::to_string(pid);
            if (name.size() > 32) name.resize(32);
            std::string q = "insert into products (" + std::to_string(pid)
                          + ", \"" + name + "\", " + std::to_string(qty) + ");";
            executor.execute(q, s);
            if (++products_done >= inserts_per_table) stop = true;
        }
    };

    auto message_worker = [&]() {
        Session s;
        s.session_id = 3;
        s.client_fd  = -1;
        unsigned int seed = static_cast<unsigned int>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()) + 2000000u;
        while (!stop) {
            seed = seed * 1664525u + 1013904223u;
            unsigned int mid       = seed % 1'999'000'000u + 1u;
            unsigned int sender_id = (seed ^ (seed >> 13)) % 1'999'000'000u + 1u;
            std::string  text      = "msg" + std::to_string(mid);
            if (text.size() > 32) text.resize(32);
            std::string q = "insert into messages (" + std::to_string(mid)
                          + ", \"" + text + "\", " + std::to_string(sender_id) + ");";
            executor.execute(q, s);
            if (++messages_done >= inserts_per_table) stop = true;
        }
    };

    // Print header
    std::cout << std::right
              << std::setw(6)  << "Time"     << " | "
              << std::setw(8)  << "Users"    << " | "
              << std::setw(7)  << "U-QPS"    << " | "
              << std::setw(8)  << "Products" << " | "
              << std::setw(7)  << "P-QPS"    << " | "
              << std::setw(8)  << "Messages" << " | "
              << std::setw(7)  << "M-QPS"    << " | "
              << std::setw(9)  << "Total"    << " | "
              << std::setw(7)  << "Avg QPS"  << "\n";
    std::cout << std::string(88, '-') << "\n";

    auto t0      = std::chrono::steady_clock::now();
    int  last_u  = 0, last_p = 0, last_m = 0;
    auto last_ts = t0;

    std::thread ut(user_worker);
    std::thread pt(product_worker);
    std::thread mt(message_worker);

    while (!stop) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto   now      = std::chrono::steady_clock::now();
        double elapsed  = std::chrono::duration<double>(now - t0).count();
        double interval = std::chrono::duration<double>(now - last_ts).count();

        int u     = users_done.load();
        int p     = products_done.load();
        int m     = messages_done.load();
        int u_qps = (int)((u - last_u) / interval);
        int p_qps = (int)((p - last_p) / interval);
        int m_qps = (int)((m - last_m) / interval);
        int avg   = elapsed > 0 ? (int)((u + p + m) / elapsed) : 0;

        std::cout << std::fixed << std::setprecision(1)
                  << std::setw(6) << elapsed << "s | "
                  << std::setw(8) << u       << " | "
                  << std::setw(7) << u_qps   << " | "
                  << std::setw(8) << p       << " | "
                  << std::setw(7) << p_qps   << " | "
                  << std::setw(8) << m       << " | "
                  << std::setw(7) << m_qps   << " | "
                  << std::setw(9) << (u+p+m) << " | "
                  << std::setw(7) << avg     << "\n";

        last_u  = u;  last_p  = p;  last_m  = m;
        last_ts = now;
    }

    ut.join();  pt.join();  mt.join();

    double total = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::cout << "\n[triplet-bench] users=" << users_done
              << " products=" << products_done
              << " messages=" << messages_done
              << " total=" << (users_done + products_done + messages_done)
              << " time=" << std::fixed << std::setprecision(2) << total << "s"
              << " avg_qps=" << (int)((users_done + products_done + messages_done) / total) << "\n";
}







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
