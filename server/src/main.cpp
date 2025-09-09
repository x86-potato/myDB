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

    //if(file_manager.root_node_pointer != 0)
        //file_manager.print_leaves(file_manager.root_node_pointer);

    
    while (true)
    {
        std::cout << "\n type key space value: ";
        std::string key_buffer;
        std::string value_buffer; 
        std::cin >> std::setw(32) >> key_buffer; 
        std::cin >> value_buffer; 

        if(key_buffer == "print")
        {
            file_manager.print_leaves(file_manager.root_node_pointer);
        }
        else if(key_buffer == "root")
        {
            std::cout << file_manager.root_node_pointer;
        }
        else
        {
            file_manager.insert_data(index_tree,key_buffer, value_buffer);
        }


        //func(index_tree,file_manager);
    }
    return 0;
}