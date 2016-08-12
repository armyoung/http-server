
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include <assert.h>

#include "log.h"
#include "alloc.h"
#include "server.h"

#include "http_engine.h"

Server* g_server_handle = NULL;

void SignalStop(int sig)
{
    LogInfo("sig %d", sig);
    if (g_server_handle)
    {
        g_server_handle->SetStop();
    }
}

void SignalCrash(int sig)
{
    LogInfo("sig %d", sig);
    DumpTrace();
}

void SignalHandler()
{
    assert(sigset(SIGINT, SignalStop) != SIG_ERR);
    assert(sigset(SIGTERM, SignalStop) != SIG_ERR);
    assert(sigset(SIGHUP, SignalStop) != SIG_ERR);
    assert(sigset(SIGUSR1, SignalStop) != SIG_ERR);

    assert(sigset(SIGTRAP, SignalCrash) != SIG_ERR);
    assert(sigset(SIGBUS, SignalCrash) != SIG_ERR);
    assert(sigset(SIGFPE, SignalCrash) != SIG_ERR);
    assert(sigset(SIGSEGV, SignalCrash) != SIG_ERR);
    assert(sigset(SIGABRT, SignalCrash) != SIG_ERR);
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("%s [listen ip] [listen port] [log out path] [html root path]\n", argv[0]);
        return 0;
    }

    SignalHandler();

    int ret = Log::Instance()->Init(argv[3]);
    if (ret < 0)
    {
        return ret;
    }

    Status::Instance();
    Alloc::Instance();

    ret = HttpEngine::Instance()->Load(argv[4]);
    if (ret < 0)
    {
        return ret;
    }

    g_server_handle = new Server(argv[1], atoi(argv[2]));

    ret = g_server_handle->StartServer();
    if (ret < 0)
    {
        // log err
        LogErr("StartServer ret %d", ret);
        return ret;
    }

    return 0;
}
