#pragma once

#include <pthread.h>

#include <vector>

#include "queue.h"

#include "io_worker.h"

class Server
{
public:
    Server(const char* ip, int port);
    ~Server();

    int StartServer();
    int StopServer()
    {
        SetStop();
    }

    void SetStop();

private:
    int InitSocket();
    void StartIOWorkers();

private:
    bool stop_;

    const char* ip_;
    int port_;

    int listen_fd_;

    Queue session_queue_;

    std::vector<IOWorker*> io_workers_;
    pthread_t* worker_tids_;
};
