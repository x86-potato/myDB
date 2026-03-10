#include "input.hpp"
#include <iostream>
#include <chrono>


CLI::CLI(Executor& executor) : executor_(executor) {}

void CLI::run(Server& server) {
    std::string line;
    
    // 1. THE INITIAL PROMPT: Print this before the loop ever starts
    std::cout << "db> ";

    while (std::getline(std::cin, line)) {
        if (line == "exit") {
            break;
        }

        if (!line.empty()) {  
            auto start_time = std::chrono::high_resolution_clock::now();
            
            //executor_.execute(line, server.sessions.at(server.current_session_id));
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

            std::cout << "\nExecution time: " << elapsed.count() << " µs\ndb> " << std::flush;

        }


        std::cout << "db> ";
    }
}