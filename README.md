# Websocket - A header only websocket client library for C++

A simple header only library that implements a websocket client in c++

# Usage:
See examples in the `./examples` directory.
```
#include <iostream>
#include "websocket.hpp"

void listen(char* data, int length)
{
    printf("testing:%s\n",data);
    std::cout << length << std::endl;
}

int main()
{
    websocket test;
    void (*listener)(char*, int) = listen;
    test.connectSocket("127.0.0.1", 8081, listen);
    test.sendMessage("testing:whatthebif");
    std::cout << "Sent message" << std::endl;
    test.exit();
}
```