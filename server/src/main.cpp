#include "config.h"
#include "btree.hpp"
#include "file.hpp"
#include "query.hpp"
#include "record.hpp"

#include <iostream>
#include <cstring>


#include <chrono>




using BtreePlus32 = BtreePlus<Node32, LeafNode32, InternalNode32>;
using BtreePlus8  = BtreePlus<Node8, LeafNode8,InternalNode8>;





int main()
{
    File file;

    BtreePlus8 index_tree8(file);
    BtreePlus32 index_tree32(file);

    std::string line; 
    StringVec lines;

    //std::ifstream file("tests/users.sql");

    while (std::getline(std::cin, line)) {
        if(line == "exit") break;
        lines.push_back(line);
    }

    // Start timing here
    auto start_time = std::chrono::high_resolution_clock::now();

    for (const auto& input : lines) 
    {
        Query::execute(input,file, &index_tree32, &index_tree8);
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    std::cout << "Execution time: " << elapsed.count() << " seconds\n";

    file.cache.flush_cache();

    return 0;
}


