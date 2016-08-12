

#pragma once

#include <queue>

#include "configuration.h"

#include "util.h"

class Alloc
{
public:
    static Alloc* Instance()
    {
        static Alloc alloc;
        return &alloc;
    }

    ~Alloc()
    {
        while (!request_buffers_.empty())
        {
            char* request_buffer = request_buffers_.front();
            delete[] request_buffer;
            request_buffers_.pop();
        }

        while (!response_buffers_.empty())
        {
            char* response_buffer = response_buffers_.front();
            delete[] response_buffer;
            response_buffers_.pop();
        }
    }

    void Acquire(char*& request_buffer, char*& response_buffer)
    {
        SmartLock locker(mutex_);
        request_buffer  = request_buffers_.front();
        response_buffer = response_buffers_.front();
        if (request_buffer && response_buffer)
        {
            request_buffers_.pop();
            response_buffers_.pop();
            return;
        }
        else if (!request_buffer && !response_buffer)
        {
            request_buffer  = new char[SESSION_READ_BUFFER_SIZE]();
            response_buffer = new char[SESSION_WRITE_BUFFER_SIZE]();
            assert(request_buffer && response_buffer);
            return;
        }
        else
        {
            assert(0);
        }
    }

    void Return(char*& request_buffer, char*& response_buffer)
    {
        if (!request_buffer && !response_buffer)
        {
            return;
        }

        SmartLock locker(mutex_);
        request_buffers_.push(request_buffer);
        request_buffer = NULL;

        response_buffers_.push(response_buffer);
        response_buffer = NULL;
    }

private:
    Alloc()
    {
        for (int index = 0; index < CONNECTIONS_NUM_MAX; ++index)
        {
            char* request_buffer = new char[SESSION_READ_BUFFER_SIZE]();
            assert(request_buffer);
            request_buffers_.push(request_buffer);

            char* response_buffer = new char[SESSION_WRITE_BUFFER_SIZE]();
            assert(response_buffer);
            response_buffers_.push(response_buffer);
        }

        assert(0 == pthread_mutex_init(&mutex_, NULL));
    }

private:
    std::queue<char*> request_buffers_;
    std::queue<char*> response_buffers_;
    pthread_mutex_t mutex_;
};
