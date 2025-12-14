#include "config.h"
#include "core/database.hpp" 
#include "core/btree.hpp"
#include "storage/file.hpp"
//#include "query/querylegacy.hpp"
#include "storage/record.hpp"


#include "query/lexer.hpp"  

#include "query/executor.hpp"

#include <iostream>
#include <cstring>


#include <chrono>









int main()
{
    Database database;
    Executor executor(database);
    // define index trees
    std::string line; 
    StringVec lines;

    std::cout << "db> ";
    while (std::getline(std::cin, line)) {
        if (line == "exit") break;

        auto start_time = std::chrono::high_resolution_clock::now();

        executor.execute(line); 


        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        std::cout << "\nExecution time: " << elapsed.count() << " µs\n";
        std::cout << "db> ";
    }




    database.flush();

    return 0;
}


