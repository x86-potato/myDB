//right now simulates multiple sessions via switch statement, via cli
#pragma once

#include "../config.h"
#include "session.hpp"
#include <unordered_map>
#include "../core/database.hpp"
#include "../cli/input.hpp"
#include <thread>
#include <memory>
#include <mutex>
#include <atomic>

class CLI;
class Database;
class Executor;
class Session;

struct Server
{
    Database& database;
    Executor &executor;

    Session admin_session; // Session for the admin CLI
    std::unordered_map<int, std::shared_ptr<Session>> active_sessions;
    
    
    int server_fd = -1; // File descriptor for the server socket
    std::mutex session_mutex; // Mutex to protect access to active_sessions
    std::atomic<int> session_id_counter{1}; //Reserve 0 for admin Atomic counter for generating unique session IDs


    Server(Database& database, Executor& executor) : database(database), executor(executor) {}

    void start();
    void add_session(int client_fd);
    void remove_session(int session_id);

};