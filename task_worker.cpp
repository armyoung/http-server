#include "task_worker.h"

#include "session.h"

#include "dispatcher.h"

#include "log.h"

TaskWorker::TaskWorker(Queue& task_queue, Queue& response_queue)
    : task_queue_(task_queue), response_queue_(response_queue)
{
    stop_ = false;
}

TaskWorker::~TaskWorker()
{
    stop_ = true;
}

int TaskWorker::Run()
{
    LogInfo("start");
    uint64_t loop = 0;
    uint64_t slow = 0;
    uint64_t used = 0;
    while (!stop_)
    {
        void* item = task_queue_.Pop();
        assert(NULL != item);

        Session* client = reinterpret_cast<Session*>(item);
        client->CheckDispatch();

        Slice request  = client->RequestBuffer();
        Slice response = client->ResponseBuffer();

        Timer watch;
        Dispatcher::Dispatch(request, response);
        client->CommitResponse(response.size);

        response_queue_.Push(item);
        client->CheckResponse();

        uint64_t ms_used = watch.Read();
        LogDebug("task from client %s %d request.size %zu response.size %zu time %llu ms",
                 client->IP(),
                 client->FileDescriptor(),
                 request.size,
                 response.size,
                 ms_used);

        used += ms_used;
        if (ms_used > 10)
        {
            slow++;
        }

        if (loop++ % 1000 == 0)
        {
            LogInfo("loop %llu used %llu slow %llu", loop, used, slow);
        }

        StatusAddTask();
    }

    LogInfo("exit");
    return 0;
}
