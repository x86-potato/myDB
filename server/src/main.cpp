#include "btree.hpp"
#include "file.hpp"
#include "lexer.hpp"
#include "query.hpp"
#include "record.hpp"

#include <iostream>
#include <cstring>


#include <chrono>



using BtreePlus32 = BtreePlus<Node32, LeafNode32, InternalNode32>;
using BtreePlus8  = BtreePlus<Node8, LeafNode8,InternalNode8>;





int main()
{
    File file_manager;

    BtreePlus8 index_tree8(file_manager);
    BtreePlus32 index_tree32(file_manager);

    std::string line; 
    std::vector<std::string> lines;

    std::ifstream file("tests/users.sql");

    while (std::getline(std::cin, line)) {
        if(line == "exit") break;
        lines.push_back(line);
    }

    // Start timing here
    auto start_time = std::chrono::high_resolution_clock::now();

    for (const auto& input : lines) {

        Query query(input, &index_tree32, &index_tree8);
        
        query.execute(file_manager);

        

        
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    std::cout << "Execution time: " << elapsed.count() << " seconds\n";

    file_manager.cache.flush_cache();

    return 0;
}


