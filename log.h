#pragma once

#include <stdio.h>

#include <inttypes.h>

#define LOG(prefix, format, ...) \
    Log::Instance()->LogImpl(    \
        "[ %s %s %d ] " prefix format "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)

#define LogErr(format, ...) LOG("ERR: ", format, ##__VA_ARGS__)
#define LogDebug(format, ...) LOG("DEBUG: ", format, ##__VA_ARGS__)
#define LogInfo(format, ...) LOG("INFO: ", format, ##__VA_ARGS__)

class Log
{
public:
    static Log* Instance()
    {
        static Log inst;
        return &inst;
    }
    ~Log();

    int Init(const char* path);

    void LogImpl(const char* format, ...);

private:
    Log();

private:
    const char* path_;
    FILE* fp_;
};
