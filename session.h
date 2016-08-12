
#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <string>

#include "util.h"
#include "queue.h"
#include "configuration.h"

#include "status.h"
#include "log.h"

#include "alloc.h"

class Session
{
public:
    Session(int fd, struct sockaddr_in addr, socklen_t len);
    ~Session();

    int FileDescriptor()
    {
        return fd_;
    }

    int Read();
    int Write();

    const char* IP()
    {
        return ip_.c_str();
    }

    Slice RequestBuffer()
    {
        return Slice(read_buffer_, read_buffer_cursor_);
    }

    Slice ResponseBuffer()
    {
        return Slice(write_buffer_, SESSION_WRITE_BUFFER_SIZE);
    }

    void CommitResponse(int size)
    {
        write_buffer_size_ = size;
    }

    void Reset();

public:
    enum
    {
        READ_NULL       = 0,
        READ_NEED_MORE  = 1,
        READ_DONE       = 2,
        WRITE_NEED_MORE = 3,
        WRITE_DONE      = 4,
        WRITE_NONE      = 5,
    };

private:
    int fd_;
    struct sockaddr_in addr_;
    socklen_t len_;

    std::string ip_;

    char* read_buffer_;
    int read_buffer_cursor_;

    char* write_buffer_;
    int write_buffer_size_;
    int write_buffer_cursor_;

private:
    uint64_t accept_tick_;
    uint64_t addto_epollin_tick_;
    uint64_t addto_task_tick_;
    uint64_t dispatch_tick_;
    uint64_t addto_response_tick_;
    uint64_t addto_epollout_tick_;

    uint64_t rpc_start_tick_;
    uint64_t rpc_end_tick_;

public:
    void CheckAccept()
    {
        accept_tick_ = GetTick();
    }

    void CheckEpollIn()
    {
        addto_epollin_tick_ = GetTick();
    }

    void CheckTaskIn()
    {
        addto_task_tick_ = GetTick();
    }

    void CheckDispatch()
    {
        dispatch_tick_ = GetTick();
    }

    void CheckResponse()
    {
        addto_response_tick_ = GetTick();
    }

    void CheckEpollOut()
    {
        addto_epollout_tick_ = GetTick();
    }
};

inline void DestroySessionQueue(Queue& queue)
{
    void* item = NULL;
    while ((item = queue.TryPop()) != NULL)
    {
        Session* client = reinterpret_cast<Session*>(item);
        delete client;
    }
}
