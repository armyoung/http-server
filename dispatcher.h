

#pragma once

#include <memory.h>

#include "util.h"

#include "protocol.h"

#include "http_engine.h"

class Dispatcher
{
public:
    static void Dispatch(Slice request, Slice& response)
    {
// example

#if 0
        const char* text = HttpEngine::Instance()->Text();
        strncpy(response.data, text, response.size);
        printf("response.data %s\n", response.data);
        response.Resize(strlen(text) + 1);

#else
        memcpy(response.data, request.data + sizeof(Header), request.size - sizeof(Header));
        response.Resize(request.size - sizeof(Header));
#endif
        LogDebug("out request.size %zu response.size %zu", request.size, response.size);
    }
};
