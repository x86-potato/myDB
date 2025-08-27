#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include <sys/stat.h>


enum status
{
    EMPTY,
    READY
};


class File
{
//header
public:
    std::string file_name = "database.db"; 
    int index_block_count;
    int data_blocks_count;
    status file_status = EMPTY; 


    File()
    {
        index_block_count = 0;
        data_blocks_count = 0;

        struct stat st;
        stat(file_name.c_str(), &st);


        FILE* file = fopen(file_name.c_str(),"r+b");
        if (open_file(file) != 1)
        {
            if(st.st_size > 0)
            {
                file_status = READY;
            }
            else
            {
                file_status = EMPTY;
            } 
        }

        if (file_status == EMPTY)
        {
            create_header(file);
        }



        fclose(file);

        
    }

    int addData(char* data, int length)
    {
        FILE* file = fopen(file_name.c_str(),"r+b");

        fseek(file, 0,SEEK_END);

        fwrite(data, sizeof(data[0]), length, file);

        fclose(file);
    }

private:
    int open_file(FILE* file)
    {
        if (!file)
        {
            std::cerr << "couldnt open file \n";
            return 1;
        }

        return 0;


    }
    void create_header(FILE* file)
    {
        char header_data[] = "nopee"; 
        fwrite(header_data,sizeof(header_data[0]), 5,file);
    }
};


int main()
{
    File my_file;

    while (true)
    {
        std::string x;
        std::cin >> x; 

        my_file.addData(&x[0], x.length()); 

    }
    return 0;
}