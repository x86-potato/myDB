#include <iostream>
#include "btree.hpp"
#include "file_system.hpp"
#include <cstring>
#include <iomanip>

int func(BtreePlus &myTree, File &file);

int main()
{
    File file_manager;
    BtreePlus index_tree(file_manager);
    //func(index_tree,file_manager);

    if(file_manager.root_node_pointer != 0)
        file_manager.print_leaves(file_manager.root_node_pointer);

    
    while (true)
    {
        std::string buffer;
        std::cin >> std::setw(32) >> buffer; 

        if(buffer == "print")
        {
            file_manager.print_leaves(file_manager.root_node_pointer);
        }
        else if(buffer == "root")
        {
            std::cout << file_manager.root_node_pointer;
        }
        else
        {
            file_manager.insert_data(index_tree,buffer, 0);
        }

        //func(index_tree,file_manager);
    }
    return 0;
}