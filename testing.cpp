#include <iostream>
#include "websocket.hpp"

void listener(char* data, int length)
{
    printf("%s\n", data);
}

int main()
{
    std::string testing = "wss://echo.websocket.org/";
    std::string host = "echo.websocket.org";
    websocket w;
    int a = w.connectSocket(host, 443, listener);
    std::cout << a << std::endl;
    w.sendMessage("testing");
    w.exit();
}
