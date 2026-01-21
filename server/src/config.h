#pragma once
#include <vector>
#include <string>





constexpr int CACHE_PAGE_LIMIT = 300000;
constexpr double CACHE_GB_LIMIT = CACHE_PAGE_LIMIT * 4096.0 / (1024.0 * 1024.0 * 1024.0); // in GB
constexpr size_t PAGE_SIZE = 4096;



constexpr int MaxKeys_4 = 339;
constexpr int MaxKeys_8  = 254;  //254
constexpr int MaxKeys_16 = 169;
constexpr int MaxKeys_32 = 101; //pad 24 for 101 pad 3904 for 4

constexpr int BLOCK_SIZE = 4096;


constexpr int MAX_COLUMN_NAME = 16;



using StringVec = std::vector<std::string>;
