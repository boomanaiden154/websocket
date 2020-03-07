#include <iostream>
#include "websocket.hpp"

int main()
{
    std::string testing = "wss://127.0.0.1/";
    websocket w;
    struct websocket::url testingParsed = w.parseUrl(testing);
    std::cout << testingParsed.protocol << std::endl;
    std::cout << testingParsed.host << std::endl;
    std::cout << testingParsed.path << std::endl;
}
