
#pragma once

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <execinfo.h>

#include <assert.h>

#include "status.h"
#include "log.h"

#define TIME(NAME)         \
    {                      \
        Timer timer(NAME); \
    }

class SmartLock
{
public:
    SmartLock(pthread_mutex_t& lock) : _lock(lock)
    {
        assert(0 == pthread_mutex_lock(&_lock));
    }
    ~SmartLock()
    {
        assert(0 == pthread_mutex_unlock(&_lock));
    }

private:
    pthread_mutex_t& _lock;
};

struct Slice
{
    char* data;
    size_t size;

    Slice(char* base, size_t len) : data(base), size(len)
    {
    }

    void Resize(size_t len)
    {
        size = len;
    }
};

inline uint64_t counter(void)
{
    register uint32_t low, high;
    register uint64_t tmp;
    __asm__ __volatile__("rdtscp" : "=a"(low), "=d"(high));
    tmp = high;
    tmp <<= 32;
    return (tmp | low);
}

inline uint64_t CPUKHZ()
{
    FILE* fp = fopen("/proc/cpuinfo", "r");
    if (!fp)
    {
        LogErr("fopen /proc/cpuinfo err %s", strerror(errno));
        return 1;
    }

    char buf[4096] = {0};
    fread(buf, 1, sizeof(buf), fp);
    fclose(fp);

    char* cursor = strstr(buf, "cpu MHz");
    if (!cursor)
    {
        return 1;
    }
    cursor += strlen("cpu MHz");
    while (*cursor == ' ' || *cursor == '\t' || *cursor == ':')
    {
        ++cursor;
    }

    double mhz   = atof(cursor);
    uint64_t khz = (uint64_t)(mhz * 1000);

    LogInfo("mhz %0.2f khz %llu", mhz, khz);
    return khz;
}

inline uint32_t GetKHZ()
{
    static uint32_t khz = CPUKHZ();
    return khz;
}

inline uint64_t GetMicroSecond()
{
    return counter() / GetKHZ();
}

inline uint64_t GetTick()
{
    return counter();
}

class Timer
{
public:
    Timer()
    {
        start_ = GetMicroSecond();
    }
    ~Timer()
    {
        if (func_)
        {
            uint64_t now   = GetMicroSecond();
            uint64_t delta = now - start_;
            if (delta != 0)
            {
                LogInfo("%s %llu ms", func_, delta);
            }
        }
    }

    Timer(const char* func) : func_(func)
    {
        start_ = GetMicroSecond();
    }

    uint64_t Read()
    {
        uint64_t now = GetMicroSecond();
        return now - start_;
    }

private:
    uint64_t start_;
    const char* func_;
};

#define DUMP_STACK_DEPTH_MAX 16

inline void DumpTrace()
{
    void* stack_trace[DUMP_STACK_DEPTH_MAX] = {0};
    char** stack_strings                    = NULL;
    int stack_depth                         = 0;

    stack_depth = backtrace(stack_trace, DUMP_STACK_DEPTH_MAX);

    stack_strings = (char**)backtrace_symbols(stack_trace, stack_depth);
    if (NULL == stack_strings)
    {
        LogErr(" Memory is not enough while dump Stack Trace!");
        return;
    }

    LogInfo("Stack Trace:");
    for (int index = 0; index < stack_depth; ++index)
    {
        LogDebug(" [%d] %s", index, stack_strings[index]);
    }

    free(stack_strings);
    stack_strings = NULL;
    return;
}
