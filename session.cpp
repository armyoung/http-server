
#include <memory.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "session.h"

#include "protocol.h"

#include "log.h"
#include "util.h"

#define __error_line (-12000 - __LINE__)

Session::Session(int fd, struct sockaddr_in addr, socklen_t len) : fd_(fd), len_(len)
{
    memcpy(&addr_, &addr, sizeof(addr));

    Alloc::Instance()->Acquire(read_buffer_, write_buffer_);
    read_buffer_cursor_  = 0;
    write_buffer_size_   = 0;
    write_buffer_cursor_ = 0;

    ip_ = inet_ntoa(addr.sin_addr);

    accept_tick_         = 0;
    addto_epollin_tick_  = 0;
    addto_task_tick_     = 0;
    dispatch_tick_       = 0;
    addto_response_tick_ = 0;
    addto_epollout_tick_ = 0;
    rpc_start_tick_      = 0;
    rpc_end_tick_        = 0;
}

Session::~Session()
{
    Alloc::Instance()->Return(read_buffer_, write_buffer_);
    read_buffer_  = NULL;
    write_buffer_ = NULL;

    close(fd_);
}

int Session::Read()
{
    if (rpc_start_tick_ == 0)
    {
        rpc_start_tick_ = GetTick();
    }

#if 1
    size_t len = read(fd_, read_buffer_, SESSION_READ_BUFFER_SIZE);
    if (len < 0)
    {
        return __error_line;
    }
    if (0 == len)
    {
        return READ_NULL;
    }

    read_buffer_cursor_ += len;
    printf("read_buffer_:\n%s\n", read_buffer_);
    return READ_DONE;

#else
    if (read_buffer_cursor_ < sizeof(Header))
    {
        // read an intact header first
        size_t to_read = sizeof(Header) - read_buffer_cursor_;

        size_t len = read(fd_, read_buffer_ + read_buffer_cursor_, to_read);
        if (len < 0)
        {
            LogErr("read %d to_read %zu len %zu err %s", fd_, to_read, len, strerror(errno));
            return __error_line;
        }
        if (0 == len)
        {
            return READ_NULL;
        }

        read_buffer_cursor_ += len;
        if (len < to_read)
        {
            return READ_NEED_MORE;
        }

        if (len > to_read)
        {
            LogErr("weird buffer len %zu to_read %zu", len, to_read);
            return __error_line;
        }
    }

    // here, we must have an intact header
    Header* header = reinterpret_cast<Header*>(read_buffer_);
    size_t to_read = header->size - read_buffer_cursor_ + sizeof(Header);
    size_t len     = read(fd_, read_buffer_ + read_buffer_cursor_, to_read);
    if (len < 0)
    {
        LogErr("read %d to_read %zu len %zu err %s", fd_, to_read, len, strerror(errno));
        return __error_line;
    }
    if (0 == len)
    {
        return READ_NULL;
    }

    read_buffer_cursor_ += len;
    if (len < to_read)
    {
        return READ_NEED_MORE;
    }

    assert(read_buffer_cursor_ = sizeof(Header) + header->size);
    return READ_DONE;
#endif
}

int Session::Write()
{
    if (write_buffer_size_ == write_buffer_cursor_)
    {
        // nothing to write
        return WRITE_NONE;
    }

    size_t len =
        write(fd_, write_buffer_ + write_buffer_cursor_, write_buffer_size_ - write_buffer_cursor_);
    if (len < 0)
    {
        LogErr("write %d size %d err %s",
               fd_,
               write_buffer_size_ - write_buffer_cursor_,
               strerror(errno));
        return __error_line;
    }

    write_buffer_cursor_ += len;
    LogDebug("write %d cursor %d size %d", fd_, write_buffer_cursor_, write_buffer_size_);
    if (write_buffer_cursor_ == write_buffer_size_)
    {
        rpc_end_tick_ = GetTick();

        return WRITE_DONE;
    }
    else if (write_buffer_cursor_ < write_buffer_size_)
    {
        return WRITE_NEED_MORE;
    }
    else
    {
        // log
        LogErr("weird buffer");
        return __error_line;
    }
}

void Session::Reset()
{
    uint64_t now         = GetTick();
    uint64_t epollin_ms  = (addto_task_tick_ - addto_epollin_tick_) / GetKHZ();
    uint64_t task_ms     = (dispatch_tick_ - addto_task_tick_) / GetKHZ();
    uint64_t dispatch_ms = (addto_response_tick_ - dispatch_tick_) / GetKHZ();
    uint64_t response_ms = (addto_epollout_tick_ - addto_response_tick_) / GetKHZ();
    uint64_t epollout_ms = (now - addto_epollout_tick_) / GetKHZ();
    uint64_t rpc_ms      = (rpc_end_tick_ - rpc_start_tick_) / GetKHZ();

    LogInfo("ip %s fd %d epollin %llu task %llu dispath %llu response %llu epollout %llu rpc %llu",
            IP(),
            fd_,
            epollin_ms,
            task_ms,
            dispatch_ms,
            response_ms,
            epollout_ms,
            rpc_ms);

    StatusTick(Epollin, epollin_ms);
    StatusTick(Task, task_ms);
    StatusTick(Dispatch, dispatch_ms);
    StatusTick(Response, response_ms);
    StatusTick(Epollout, epollout_ms);

    read_buffer_cursor_ = 0;

    write_buffer_size_   = 0;
    write_buffer_cursor_ = 0;

    addto_epollin_tick_  = 0;
    addto_task_tick_     = 0;
    dispatch_tick_       = 0;
    addto_response_tick_ = 0;
    addto_epollout_tick_ = 0;

    rpc_start_tick_ = 0;
    rpc_end_tick_   = 0;
}
