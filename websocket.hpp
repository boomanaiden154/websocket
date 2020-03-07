#pragma once

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <bitset>
#include <random>
#include <climits>
#include <pthread.h>
#include <errno.h>
#include <openssl/ssl.h>
#include <time.h>

class timer
{
    struct waitThenCallArguments
    {
        uint32_t toWait;
        void (*callbackFunction)();
    };

    static void waitMilliseconds(uint32_t toWait)
    {
        struct timespec ts;
        ts.tv_sec = toWait / 1000;
        ts.tv_nsec = (toWait % 1000) * 1000000;
        nanosleep(&ts, &ts);
    }

    static void* waitThenCall(void* arguments)
    {
        waitThenCallArguments* args = (waitThenCallArguments*)arguments;
        waitMilliseconds(args->toWait);
        args->callbackFunction();
        return NULL;
    }

    static void* waitThenCallRepeat(void* arguments)
    {
        while(true)
        {
            waitThenCall(arguments);
        }
    }
public:
    static pthread_t asyncTimer(uint32_t toWait, void (*callbackFunction)())
    {
        waitThenCallArguments* arguments = (waitThenCallArguments*)malloc(sizeof(waitThenCallArguments));
        arguments->callbackFunction = callbackFunction;
        arguments->toWait = toWait;
        pthread_t thread;
        pthread_create(&thread, NULL, waitThenCall, arguments);
        return thread;
    }

    static pthread_t asyncTimerRepeat(uint32_t toWait, void (*callbackFunction)())
    {
        waitThenCallArguments* arguments = (waitThenCallArguments*)malloc(sizeof(waitThenCallArguments));
        arguments->callbackFunction = callbackFunction;
        arguments->toWait = toWait;
        pthread_t thread;
        pthread_create(&thread, NULL, waitThenCallRepeat, arguments);
        return thread;
    }
};

class websocket
{
public:
    struct listenArguments
    {
    	SSL* SSLConnection;
        void (*listenerCallback)(char*, int);
    };

    struct url
    {
        std::string protocol;
        std::string host;
        std::string path;
    };

    struct url parseUrl(std::string input)
    {
        struct url toReturn;
        for(int i = 0; i < input.size(); i++)
        {
            if(input[i] == ':' && input[i + 1] == '/' && input[i + 2] == '/')
            {
                break;
            }
            toReturn.protocol += input[i];
        }
        for(int i = toReturn.protocol.size() + 3; i < input.size(); i++)
        {
            if(input[i] == '/')
            {
                break;
            }
            toReturn.host += input[i];
        }
        for(int i = toReturn.protocol.size() + 3 + toReturn.host.size(); i < input.size(); i++)
        {
            toReturn.path += input[i];
        }
        return toReturn;
    }

    pthread_t listenerThread;
    SSL* SSLConnection;
    struct url URL;

    static void* listen(void* args)
    {
        struct listenArguments* arguments = (struct listenArguments*)args;
        SSL* SSLConnection = arguments->SSLConnection;
        void (*listenerCallback)(char*, int) = arguments->listenerCallback;
        while(true)
        {
            char socketBuffer[2];
            int bytesRecieved1 = SSL_read(SSLConnection, socketBuffer, sizeof(socketBuffer));
            uint8_t payloadLengthSimple = socketBuffer[1] & 0b01111111; //get the seven least significant bits
            uint64_t payloadLength;
            if(payloadLengthSimple <= 125)
            {
                    payloadLength = payloadLengthSimple;
            }
            else if(payloadLengthSimple == 126)
            {
                    uint8_t payloadLengthBuffer[2];
                    SSL_read(SSLConnection, payloadLengthBuffer, sizeof(payloadLengthBuffer));
                    payloadLength = (uint64_t)payloadLengthBuffer[0] << 8;
                    payloadLength += (uint64_t)payloadLengthBuffer[1];
            }
            else if(payloadLengthSimple == 127)
            {
                    uint8_t payloadLengthBuffer[8];
                    SSL_read(SSLConnection, payloadLengthBuffer, sizeof(payloadLengthBuffer));
                    payloadLength = (uint64_t)payloadLengthBuffer[0] << 56;
                    payloadLength += (uint64_t)payloadLengthBuffer[1] << 48;
                    payloadLength += (uint64_t)payloadLengthBuffer[2] << 40;
                    payloadLength += (uint64_t)payloadLengthBuffer[3] << 32;
                    payloadLength += (uint64_t)payloadLengthBuffer[4] << 24;
                    payloadLength += (uint64_t)payloadLengthBuffer[5] << 16;
                    payloadLength += (uint64_t)payloadLengthBuffer[6] << 8;
                    payloadLength += (uint64_t)payloadLengthBuffer[7];
            }
            char* textBuffer = new char[payloadLength + 1];
            uint32_t bytesRecieved = 0;
            while(bytesRecieved < payloadLength)
            {
                bytesRecieved += SSL_read(SSLConnection, textBuffer, payloadLength - bytesRecieved);
            }
            textBuffer[payloadLength] = '\0';
            listenerCallback(textBuffer, payloadLength);
        }
        pthread_exit(0);
        return NULL;
    }

    int connectSocket(std::string address, int port, void (*listenerCallback)(char*,int))
    {
        URL = parseUrl(address);
        int sockfd;
        if((sockfd = initalizeSocket(URL.host, port)) == -1)
        {
            return 1;
        }
        if(initalizeSocketSSL(sockfd) != 0)
        {
            return 2;
        }

        std::string request = "GET " + address + " HTTP/1.1\r\n"
            "Host: " + URL.host + "\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
            "Origin: http://localhost\r\n"
            "Sec-WebSocket-Protocol: chat, superchat\r\n"
            "Sec-WebSocket-Version: 13\r\n\r\n";
        SSL_write(SSLConnection, request.c_str(), request.length());

        char HTTPBuffer[1024];
        int recieved = SSL_read(SSLConnection, HTTPBuffer, sizeof(HTTPBuffer) - 1);
        //should probably process this to see if it is correct

        struct listenArguments* arguments;
        arguments = (listenArguments*)malloc(sizeof(listenArguments));
        arguments->SSLConnection = SSLConnection;
        arguments->listenerCallback = listenerCallback;

        pthread_create(&listenerThread, NULL, listen, (void*)arguments);

        return 0;
    }

    void exit()
    {
        pthread_join(listenerThread, NULL);
        pthread_exit(NULL);
    }

    void sendMessage(std::string message)
    {
        //six bytes for framing data, four bytes for length data(if necessary), and one character for each byte
        char toSend[10 + message.length()];
        //set message to not fragmented, type as text, and no special flags
        toSend[0] = 0b10000001; 
        uint8_t offset = 0;
        //set the mask to true on each of these(first bit set to 1)
        if(message.length() <= 125)
        {
            toSend[1] = 0b10000000 + message.length();
        }
        else if(message.length() >= 125 && message.length() <= SHRT_MAX)
        {
            //set mask to true and length to 126
            toSend[1] = 0b11111110;
            toSend[2] = (uint8_t)(message.length() >> 8);
            toSend[3] = (uint8_t)(message.length());
            offset = 2;
        }
        else {
            //set the length to 127 and the mask to true
            toSend[1] = 0xff;
            toSend[2] = (uint8_t)(message.length() >> 56);
            toSend[3] = (uint8_t)(message.length() >> 48);
            toSend[4] = (uint8_t)(message.length() >> 40);
            toSend[5] = (uint8_t)(message.length() >> 32);
            toSend[6] = (uint8_t)(message.length() >> 24);
            toSend[7] = (uint8_t)(message.length() >> 16);
            toSend[8] = (uint8_t)(message.length() >> 8);
            toSend[9] = (uint8_t)(message.length());
            offset = 8;
        }
        //generate the masking key
        std::random_device randomDevice;
        std::mt19937 mersenneTwister;
        std::uniform_int_distribution<uint8_t> distribution(0, 255);
        toSend[2 + offset] = (char)distribution(mersenneTwister);
        toSend[3 + offset] = (char)distribution(mersenneTwister);
        toSend[4 + offset] = (char)distribution(mersenneTwister);
        toSend[5 + offset] = (char)distribution(mersenneTwister);

        //add the data to the toSend array
        for(int i = 0; i < message.length(); i++)
        {
            //xor the byte with the masking key to "mask" the message
            toSend[6 + offset + i] = message[i] ^ toSend[2 + offset + i % 4];
        }

        SSL_write(SSLConnection, toSend, offset + 6 + message.length());
    }

    int initalizeSocket(std::string address, int port)
    {
        int sockfd;

        struct addrinfo hints;
        struct addrinfo* servinfo;
        struct addrinfo* p;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        getaddrinfo(address.c_str(), std::to_string(port).c_str(), &hints, &servinfo);

        for(p = servinfo; p != NULL; p = p->ai_next)
        {
            if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            {
                    return -1;
                    continue;
            }
            if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
            {
                    close(sockfd);
                    return -1;
                    continue;
            }
            break;
        }

        if(p == NULL)
        {
            return -1;
        }

        freeaddrinfo(servinfo);

        return sockfd;
    }

    int initalizeSocketSSL(int sockfd)
    {
        SSL_library_init();
        SSL_CTX* SSLContext = SSL_CTX_new(SSLv23_client_method());
        SSLConnection = SSL_new(SSLContext);
        SSL_set_fd(SSLConnection, sockfd);

        int errorCode = SSL_connect(SSLConnection);

        if(errorCode != 1)
        {
            return errorCode;
        }

        return 0;
    }
};
