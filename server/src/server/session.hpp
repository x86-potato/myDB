//session defs
#pragma once
#include "../config.h"
#include <thread>
#include <unistd.h>

class Server;

struct Session {
    int session_id = -1;
    int client_fd = -1;
    int current_transaction_id = -1; // -1 means no transaction



    bool is_admin() const {
        return session_id == 0; // Admin session has a reserved ID of 0
    }

    std::string read_query();

    Session() = default;

    void send_response(const std::string& response) {
        write(client_fd, response.c_str(), response.size());
        write(client_fd, "\n", 1); 
    }

    void close_session() {
        if(client_fd != -1) {
            close(client_fd);
            client_fd = -1;
        }
    }

    void run(Server& server); 
    void run_admin(Server& server); // Separate method for admin CLI

    void set_current_txn(int txn_id) {
        current_transaction_id = txn_id;
    }

    void close_session(Server &server);
};