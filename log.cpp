#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include <stdarg.h>

#include "log.h"

#define __error_line (-13000 - __LINE__)

Log::Log()
{
    path_ = NULL;
    fp_   = NULL;
}

Log::~Log()
{
    if (fp_)
    {
        fclose(fp_);
    }
}

int Log::Init(const char* path)
{
    path_ = path;

    fp_ = fopen(path, "a");
    if (fp_ == NULL)
    {
        printf("open %s err %s\n", path_, strerror(errno));
        return __error_line;
    }
    return 0;
}

void Log::LogImpl(const char* format, ...)
{
    static __thread char buffer[1024] = {0};

    time_t now    = time(NULL);
    struct tm fmt = {0};
    localtime_r(&now, &fmt);

    size_t index = strftime(buffer, sizeof(buffer), "%F %T ", &fmt);

    va_list arg;
    va_start(arg, format);
    vsnprintf(buffer + index, sizeof(buffer) - index, format, arg);
    va_end(arg);

    fputs(buffer, fp_);

    fflush(fp_);
}
