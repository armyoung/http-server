#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "server.h"

#include "session.h"

#include "io_worker.h"

#include "log.h"

#define __error_line (-10000 - __LINE__)

Server::Server(const char* ip, int port) : ip_(ip), port_(port)
{
    stop_ = false;

    listen_fd_ = -1;

    worker_tids_ = NULL;
}

Server::~Server()
{
    SetStop();

    for (size_t index = 0; index < io_workers_.size(); ++index)
    {
        delete io_workers_[index];
    }

    delete[] worker_tids_;

    if (listen_fd_ != -1)
    {
        close(listen_fd_);
    }

    DestroySessionQueue(session_queue_);
}

int Server::InitSocket()
{
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0)
    {
        LogErr("socket err %s", strerror(errno));
        return __error_line;
    }

    struct sockaddr_in addr;
    // init addr
    {
        bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip_, &addr.sin_addr);
        addr.sin_port = htons(port_);
    }

    if (bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        LogErr("bind %d err %s", listen_fd_, strerror(errno));
        return __error_line;
    }

    if (listen(listen_fd_, 10) < 0)
    {
        LogErr("listen %d err %s", listen_fd_, strerror(errno));
        return __error_line;
    }

    return 0;
}

int Server::StartServer()
{
    int ret = InitSocket();
    if (ret < 0)
    {
        LogErr("InitSocket fail ret %d", ret);
        return ret;
    }

    LogInfo("InitSocket success");

    StartIOWorkers();

    LogInfo("StartIOWorkers done");

    while (!stop_)
    {
        struct sockaddr_in client;
        socklen_t len  = sizeof(struct sockaddr_in);
        int connection = accept(listen_fd_, (struct sockaddr*)&client, &len);
        if (connection < 0)
        {
            LogErr("accept %d ret %d err %s", listen_fd_, connection, strerror(errno));
            return __error_line;
        }
        else
        {
            Timer timer;

            Session* session = new Session(connection, client, len);
            session->CheckAccept();

            session_queue_.Push(session);

            // single thread
            StatusAddAccept();

            LogInfo("accept new session from %s fd %d timer %llu ms",
                    session->IP(),
                    connection,
                    timer.Read());
        }
    }
}

static void* IOWorkerInstruction(void* in)
{
    IOWorker* worker = reinterpret_cast<IOWorker*>(in);
    int ret          = worker->Run();
    if (ret < 0)
    {
        // log err
        LogErr("IOWorker::Run() ret %d", ret);
        assert(0);
    }

    return NULL;
}

void Server::StartIOWorkers()
{
    LogInfo("IOWorker %lu start", pthread_self());
    worker_tids_ = new pthread_t[IO_WORKER_COUNT];

    for (int index = 0; index < IO_WORKER_COUNT; ++index)
    {
        IOWorker* worker = new IOWorker(session_queue_);

        assert(0 == pthread_create(&worker_tids_[index],
                                   NULL,
                                   IOWorkerInstruction,
                                   reinterpret_cast<void*>(worker)));

        io_workers_.push_back(worker);
    }
}

void Server::SetStop()
{
    LogInfo("called");

    stop_ = true;
    for (size_t index = 0; index < io_workers_.size(); ++index)
    {
        io_workers_[index]->SetStop();
    }

    for (size_t index = 0; index < io_workers_.size(); ++index)
    {
        pthread_join(worker_tids_[index], NULL);
    }
}
