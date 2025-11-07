#include "config.h"
#include "btree.hpp"
#include "file.hpp"
#include "query.hpp"
#include "record.hpp"

#include <iostream>
#include <cstring>


#include <chrono>









int main()
{
    File file;

    // define index trees
    MyBtree32 index_tree32(file);
    MyBtree16 index_tree16(file);
    MyBtree8 index_tree8(file);
    MyBtree4 index_tree4(file);

    std::string line; 
    StringVec lines;

    std::cout << "db> ";
    while (std::getline(std::cin, line)) {
        if(line == "exit") break;

        auto start_time = std::chrono::high_resolution_clock::now();

        Query::execute(line,file, &index_tree32, &index_tree8, &index_tree4, &index_tree16);

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end_time - start_time;
        std::cout << "\nExecution time: " << elapsed.count() << " seconds\n";

        std::cout << "db> ";
    }




    file.cache.flush_cache();

    return 0;
}


