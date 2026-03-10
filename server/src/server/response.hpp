
#include <string>
#include <cstdint>

enum class ResponseType : uint8_t {
    OK,
    DATA,
    ERROR,
    AFFECTED
};


struct Response
{
    ResponseType status_code;

    int message_length;
    std::string message; 


    

};