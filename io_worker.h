#pragma once

#include <sys/epoll.h>
#include <pthread.h>

#include <vector>

#include "queue.h"

#include "task_worker.h"

class IOWorker
{
public:
    IOWorker(Queue& sessions);
    ~IOWorker();

    int Run();

    void SetStop();

private:
    void StartTaskWorkers();

    void HandleRequests();
    void HandleResponses();

    int AddRequest(void* session);
    int AddResponse(void* session);
    int HandleEvents(struct epoll_event* events, int count);
    int HandleRead(struct epoll_event event);
    int HandleWrite(struct epoll_event event);

    void DestroySession(void* session, int state);

private:
    bool stop_;

    int epoll_fd_;

    Queue& session_queue_;

    Queue task_queue_;
    Queue response_queue_;

    std::vector<TaskWorker*> task_workers_;
    pthread_t* worker_tids_;
};
