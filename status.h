

#pragma once

#include "log.h"

#define StatusAddAccept() Status::Instance()->AddAccept()
#define StatusAddTask() Status::Instance()->AddTask()
#define StatusAddDestroy() Status::Instance()->AddDestroy()
#define StatusAddErr() Status::Instance()->AddErr()
#define StatusAddReadNext() Status::Instance()->AddReadNext()
#define StatusAddWriteNext() Status::Instance()->AddWriteNext()

#define StatusAddEvent() Status::Instance()->AddEvent()
#define StatusDelEvent() Status::Instance()->DelEvent()
#define StatusModEvent() Status::Instance()->ModEvent()

#define VARNAME(name) name##_used_tick_

#define DeclareTickFunction(Name, name)             \
    void Tick##Name(uint64_t tick)                  \
    {                                               \
        __sync_fetch_and_add(&VARNAME(name), tick); \
    }

#define FUNCNAME(Name) Tick##Name
#define StatusTick(Name, tick) Status::Instance()->FUNCNAME(Name)(tick)

class Status
{
public:
    static Status* Instance()
    {
        static Status inst;
        return &inst;
    }

    ~Status()
    {
    }

    void AddAccept()
    {
        __sync_fetch_and_add(&accept_num_, 1);
    }

    void AddTask()
    {
        __sync_fetch_and_add(&task_num_, 1);
    }

    void AddDestroy()
    {
        __sync_fetch_and_add(&destroy_num_, 1);
    }

    void AddErr()
    {
        __sync_fetch_and_add(&err_num_, 1);
    }

    void AddReadNext()
    {
        __sync_fetch_and_add(&read_next_num_, 1);
    }

    void AddWriteNext()
    {
        __sync_fetch_and_add(&write_next_num_, 1);
    }

    void AddEvent()
    {
        __sync_fetch_and_add(&add_event_num_, 1);
    }

    void DelEvent()
    {
        __sync_fetch_and_add(&del_event_num_, 1);
    }

    void ModEvent()
    {
        __sync_fetch_and_add(&mod_event_num_, 1);
    }

    // macro
    DeclareTickFunction(Accept, accept);
    DeclareTickFunction(Epollin, epollin);
    DeclareTickFunction(Task, task);
    DeclareTickFunction(Dispatch, dispatch);
    DeclareTickFunction(Response, response);
    DeclareTickFunction(Epollout, epollout);

    void PrintStatus()
    {
        LogInfo(
            "accept %llu request %llu destroy %llu err %llu "
            "read_next_num_ %llu write_next_num_ %llu "
            "accept_used_tick_ %llu epollin_used_tick_ %llu task_used_tick_ %llu "
            "dispatch_used_tick_ %llu response_used_tick_ %llu epollout_used_tick_ %llu",
            accept_num_,
            task_num_,
            destroy_num_,
            err_num_,
            read_next_num_,
            write_next_num_,
            accept_used_tick_,
            epollin_used_tick_,
            task_used_tick_,
            dispatch_used_tick_,
            response_used_tick_,
            epollout_used_tick_);

        LogInfo("add_event_num_ %llu del_event_num_ %llu mod_event_num_ %llu",
                add_event_num_,
                del_event_num_,
                mod_event_num_);
    }

    void StartPrintStatusWorker();

private:
    Status()
    {
        accept_num_  = 0;
        task_num_    = 0;
        destroy_num_ = 0;
        err_num_     = 0;

        accept_used_tick_   = 0;
        epollin_used_tick_  = 0;
        task_used_tick_     = 0;
        dispatch_used_tick_ = 0;
        response_used_tick_ = 0;
        epollout_used_tick_ = 0;

        StartPrintStatusWorker();
    }

private:
    uint64_t accept_num_;
    uint64_t task_num_;
    uint64_t destroy_num_;
    uint64_t err_num_;
    uint64_t read_next_num_;
    uint64_t write_next_num_;

    uint64_t add_event_num_;
    uint64_t del_event_num_;
    uint64_t mod_event_num_;

    uint64_t accept_used_tick_;
    uint64_t epollin_used_tick_;
    uint64_t task_used_tick_;
    uint64_t dispatch_used_tick_;
    uint64_t response_used_tick_;
    uint64_t epollout_used_tick_;
};
