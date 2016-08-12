
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include <pthread.h>

#include "status.h"

static void* PrintStatusWork(void* in)
{
    Status* status = reinterpret_cast<Status*>(in);
    while (true)
    {
        status->PrintStatus();
        sleep(1);
    }
}

void Status::StartPrintStatusWorker()
{
    pthread_t tid;
    assert(0 == pthread_create(&tid, NULL, PrintStatusWork, this));
    pthread_detach(tid);
}
