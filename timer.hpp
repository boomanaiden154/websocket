#include <time.h>
#include <pthread.h>
#include <stdint.h>

class timer
{
    struct waitThenCallArguments
    {
        uint32_t toWait;
        void (*callbackFunction)(void*);
        void* arguments;
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
        args->callbackFunction(args->arguments);
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
    static pthread_t asyncTimer(uint32_t toWait, void (*callbackFunction)(void*), void* arguments)
    {
        waitThenCallArguments* argumentsToPass = (waitThenCallArguments*)malloc(sizeof(waitThenCallArguments));
        argumentsToPass->callbackFunction = callbackFunction;
        argumentsToPass->toWait = toWait;
        argumentsToPass->arguments = arguments;
        pthread_t thread;
        pthread_create(&thread, NULL, waitThenCall, argumentsToPass);
        return thread;
    }

    static pthread_t asyncTimerRepeat(uint32_t toWait, void (*callbackFunction)(void*), void* arguments)
    {
        waitThenCallArguments* argumentsToPass = (waitThenCallArguments*)malloc(sizeof(waitThenCallArguments));
        argumentsToPass->callbackFunction = callbackFunction;
        argumentsToPass->toWait = toWait;
        argumentsToPass->arguments = arguments;
        pthread_t thread;
        pthread_create(&thread, NULL, waitThenCallRepeat, argumentsToPass);
        return thread;
    }
};
