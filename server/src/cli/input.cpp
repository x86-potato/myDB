#include "input.hpp"
#include <iostream>
#include <chrono>

CLI::CLI(Executor& executor) : executor_(executor) {}

void CLI::run() {
    std::string line;
    std::cout << "db> ";

    while (std::getline(std::cin, line)) {
        if (line == "exit") {
            break;
        }

        if (!line.empty()) {  // Skip empty lines
            auto start_time = std::chrono::high_resolution_clock::now();
            executor_.execute(line);
            auto end_time = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            std::cout << "\nExecution time: " << elapsed.count() << " µs\n";
        }


        std::cout << "db> ";
    }
}