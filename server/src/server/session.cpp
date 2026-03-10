#include "session.hpp"
#include "server.hpp"

std::string Session::read_query()
{
    std::string line;
    char c;
    while (read(client_fd, &c, 1) > 0) {
        if (c == '\n') {
            break;
        }
        line += c;
    }
    return line;
}

void Session::run(Server& server)
{
    while (true) {
        std::string query = read_query();
        if (query.empty()) {
            break; // Client disconnected
        }
        std::cout << "Received query from session " << session_id << ": " << query << std::endl;
        //current_transaction_id = server.database.create_transaction();
        server.executor.execute(query, *this);
    }
    if(client_fd != -1) {
        close(client_fd);
        client_fd = -1;
    }

}

void Session::close_session(Server &server) {
    if(client_fd != -1) {
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
        client_fd = -1;

    }
}








void Session::run_admin(Server& server) {
    std::string line;
    
    // 1. THE INITIAL PROMPT: Print this before the loop ever starts
    std::cout << "Admin CLI ready. Type 'exit' to quit.\n" << std::flush;
    std::cout << "admin>";

    while (std::getline(std::cin, line)) {
        if (line == "exit") {
            break;
        }

        if (!line.empty()) {  
            auto start_time = std::chrono::high_resolution_clock::now();
            
            server.executor.execute(line, *this);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

            std::cout << "\nExecution time: " << elapsed.count() << " µs\nadmin> " << std::flush;

        }
    }
}

