
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#include <string.h>
#include <errno.h>
#include <inttypes.h>

#include "io_worker.h"

#include "session.h"

#include "log.h"

#define __error_line (-11000 - __LINE__)

static int SetNonBlocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) != 0)
    {
        // log
        LogErr("fcntl %d err %s", fd, strerror(errno));
        return __error_line;
    }
    return 0;
}

static int AddEvent(int epoll_fd, int monitor_fd, int state, void* data)
{
    TIME(__func__);
    StatusAddEvent();

    struct epoll_event event;
    event.events   = state;
    event.data.ptr = data;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, monitor_fd, &event);
    return 0;
}

static int DeleteEvent(int epoll_fd, int monitor_fd, int state)
{
    TIME(__func__);
    StatusDelEvent();

    struct epoll_event event;
    event.events = state;
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, monitor_fd, &event);
    return 0;
}

static int ModifyEvent(int epoll_fd, int monitor_fd, int state, void* data)
{
    TIME(__func__);
    StatusModEvent();

    struct epoll_event event;
    event.events   = state;
    event.data.ptr = data;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, monitor_fd, &event);
    return 0;
}

IOWorker::IOWorker(Queue& sessions) : session_queue_(sessions)
{
    stop_ = false;

    epoll_fd_ = -1;

    worker_tids_ = NULL;
}

IOWorker::~IOWorker()
{
    SetStop();

    for (int index = 0; index < task_workers_.size(); ++index)
    {
        delete task_workers_[index];
    }

    delete[] worker_tids_;

    if (epoll_fd_ != -1)
    {
        close(epoll_fd_);
    }

    DestroySessionQueue(task_queue_);
    DestroySessionQueue(response_queue_);
}

int IOWorker::Run()
{
    StartTaskWorkers();

    epoll_fd_ = epoll_create(EPOLL_FD_MAX);
    if (epoll_fd_ < 0)
    {
        LogErr("epoll_create err %s", strerror(errno));
        return __error_line;
    }

    struct epoll_event events[EPOLL_EVENT_MAX];

    uint64_t loop = 0;
    while (!stop_)
    {
        TIME(__func__);
        if (loop++ % 1000 == 0)
        {
            LogInfo("loop %llu", loop);
        }

        // deal with in session queue
        HandleRequests();

        // deal with out queue
        HandleResponses();

        // deal with sessions
        // epoll_wait use timeout to wakeup current thread
        // actually, there is a better way
        // we can make pipe between io_worker and task_worker
        // and add these pipes into events
        // as long as task_worker finish one request, write its pipe
        // to wake up io_worker to write response
        int count = epoll_wait(epoll_fd_, events, EPOLL_EVENT_MAX, 0);
        if (-1 == count)
        {
            // log
            LogErr("epoll_wait err %s", strerror(errno));
            break;
        }

        // deal with events
        HandleEvents(events, count);
    }
    return 0;
}

void IOWorker::HandleRequests()
{
    TIME(__func__);
    {
        void* session = session_queue_.TryPop();
        if (!session)
        {
            return;
        }

        // log
        AddRequest(session);
    }
}

int IOWorker::AddRequest(void* session)
{
    TIME(__func__);
    Session* client = reinterpret_cast<Session*>(session);

    int fd  = client->FileDescriptor();
    int ret = SetNonBlocking(fd);
    if (ret < 0)
    {
        // log err
        LogErr("SetNonBlocking %d err %s", fd, strerror(errno));
        return ret;
    }

    AddEvent(epoll_fd_, fd, EPOLLIN, session);
    client->CheckEpollIn();

    LogInfo("session %s %d", client->IP(), client->FileDescriptor());
    return 0;
}

int IOWorker::HandleEvents(struct epoll_event* events, int count)
{
    TIME(__func__);
    if (count != 0)
    {
        LogInfo("count %d", count);
    }

    for (int index = 0; index < count; ++index)
    {
        if (events[index].events & EPOLLIN)
        {
            return HandleRead(events[index]);
        }
        else if (events[index].events & EPOLLOUT)
        {
            return HandleWrite(events[index]);
        }
        else
        {
            // log
            LogErr("index %d event 0x%x fd %d", index, events[index].events, events[index].data.fd);
            DestroySession(events[index].data.ptr, EPOLLIN | EPOLLOUT);
            return __error_line;
        }
    }
    return 0;
}

int IOWorker::HandleRead(struct epoll_event event)
{
    TIME(__func__);
    Session* client = reinterpret_cast<Session*>(event.data.ptr);

    int ret = client->Read();
    switch (ret)
    {
        case Session::READ_NEED_MORE:
            // wait for next in event
            StatusAddReadNext();
            break;
        case Session::READ_DONE:
            // unregistered event for session fd
            DeleteEvent(epoll_fd_, client->FileDescriptor(), EPOLLIN);
            // push into worker queue
            client->CheckTaskIn();
            task_queue_.Push(event.data.ptr);
            break;
        default:
            // log
            LogErr("client %s %d Read ret %d", client->IP(), client->FileDescriptor(), ret);
            DestroySession(event.data.ptr, EPOLLIN);
            return ret;
    }

    LogInfo("client %s %d Read ret %d", client->IP(), client->FileDescriptor(), ret);
    return 0;
}

int IOWorker::HandleWrite(struct epoll_event event)
{
    TIME(__func__);
    Session* client = reinterpret_cast<Session*>(event.data.ptr);
    // assert(client->FileDescriptor() == event.data.fd);

    int ret = client->Write();
    switch (ret)
    {
        case Session::WRITE_NEED_MORE:
            StatusAddWriteNext();
            break;
        case Session::WRITE_DONE:
            client->Reset();
            ModifyEvent(epoll_fd_, client->FileDescriptor(), EPOLLIN, event.data.ptr);
            client->CheckEpollIn();
            break;
        default:
            LogErr("client %s %d Write ret %d", client->IP(), client->FileDescriptor(), ret);
            DestroySession(event.data.ptr, EPOLLOUT);
            return ret;
    }

    LogInfo("client %s %d Write ret %d", client->IP(), client->FileDescriptor(), ret);
    return 0;
}

void IOWorker::HandleResponses()
{
    TIME(__func__);
    {
        void* item = response_queue_.TryPop();
        if (!item)
        {
            return;
        }

        AddResponse(item);
    }
}

int IOWorker::AddResponse(void* session)
{
    TIME(__func__);

    Session* client = reinterpret_cast<Session*>(session);
    client->CheckEpollOut();

    int ret = client->Write();
    switch (ret)
    {
        case Session::WRITE_NEED_MORE:
            AddEvent(epoll_fd_, client->FileDescriptor(), EPOLLOUT, session);
            break;
        case Session::WRITE_DONE:
            client->Reset();
            AddEvent(epoll_fd_, client->FileDescriptor(), EPOLLIN, session);
            client->CheckEpollIn();
            break;
        default:
            LogErr("client %s %d Write ret %d", client->IP(), client->FileDescriptor(), ret);
            DestroySession(session, 0);
            return ret;
    }

    LogInfo("client %s %d Write ret %d", client->IP(), client->FileDescriptor(), ret);
    return 0;
}

void IOWorker::DestroySession(void* session, int state)
{
    TIME(__func__);

    Session* client = reinterpret_cast<Session*>(session);
    int fd          = client->FileDescriptor();

    if (0 != state)
    {
        DeleteEvent(epoll_fd_, fd, state);
    }

    close(fd);
    delete client;

    LogInfo("session %s %d state 0x%x", client->IP(), client->FileDescriptor(), state);

    StatusAddDestroy();
}

static void* TaskWorkerInstruction(void* in)
{
    LogInfo("TaskWorker %lu start", pthread_self());
    TaskWorker* worker = reinterpret_cast<TaskWorker*>(in);
    int ret            = worker->Run();
    if (ret < 0)
    {
        // log err
        LogErr("TaskWorker::Run ret %d", ret);
        assert(0);
    }

    return NULL;
}

void IOWorker::StartTaskWorkers()
{
    worker_tids_ = new pthread_t[TASK_WORKER_COUNT_PER_IO_WORKER];
    task_workers_.reserve(TASK_WORKER_COUNT_PER_IO_WORKER);
    for (int index = 0; index < TASK_WORKER_COUNT_PER_IO_WORKER; ++index)
    {
        TaskWorker* worker = new TaskWorker(task_queue_, response_queue_);

        assert(0 == pthread_create(&worker_tids_[index],
                                   NULL,
                                   TaskWorkerInstruction,
                                   reinterpret_cast<void*>(worker)));
        pthread_detach(worker_tids_[index]);
        task_workers_.push_back(worker);
    }
}

void IOWorker::SetStop()
{
    LogInfo("called");

    stop_ = true;
}
