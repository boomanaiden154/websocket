#include <iostream>
#include "../websocket.hpp"

void listen(char* data, int length)
{
    printf("%s\n",data);
}

int main()
{
    websocket socket;
    socket.connectSocket("127.0.0.1", 8081, listen);
    socket.exit();
}