

#pragma once

#include "queue.h"
#include "util.h"

#include "log.h"

class TaskWorker
{
public:
    TaskWorker(Queue& task_queue, Queue& response_queue);
    ~TaskWorker();

    int Run();

    void SetStop()
    {
        LogInfo("called");
        stop_ = true;
    }

private:
    Queue& task_queue_;
    Queue& response_queue_;

    bool stop_;
};
