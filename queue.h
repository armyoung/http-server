#pragma once

#include <assert.h>

#include <queue>

#include "configuration.h"
#include "util.h"

class CircleQueue
{
public:
    CircleQueue(int queue_size) : head_(0), tail_(0)
    {
        queue_size_ = queue_size;
        container_  = (void**)malloc(sizeof(void*) * queue_size);
    }

    ~CircleQueue()
    {
        if (container_)
        {
            free(container_);
            container_ = NULL;
        }
    }

    int Push(void* ptr)
    {
        if (head_ - tail_ >= queue_size_)
        {
            return 1;  // full
        }

        container_[head_ % queue_size_] = ptr;

        head_ = head_ + 1;
        return 0;
    }

    bool Empty()
    {
        volatile long tail = tail_;
        if (head_ <= tail)
        {
            return true;
        }
        return false;
    }

    int Take(void** ptr, int retry)
    {
        *ptr = NULL;

        for (int i = 0; i < retry; i++)
        {
            volatile long tail = tail_;
            if (head_ <= tail)
            {
                return 1;
            }

            void* item = *(container_ + tail % queue_size_);
            int ret    = __sync_bool_compare_and_swap(&tail_, tail, tail + 1);
            if (ret)
            {
                *ptr = item;
                return 0;
            }
        }

        return -1;
    }

private:
    volatile long head_;
    volatile long tail_;
    void** container_;
    int queue_size_;
};

#if 0
class Queue
{
public:
    Queue() : queue_(EPOLL_FD_MAX)
    {
    }

    ~Queue()
    {
    }

    int Push(void* item)
    {
        int ret = queue_.Push(item);
        if (ret != 0)
        {
            LogErr("queue full");
            return ret;
        }

        return 0;
    }

    void* Pop()
    {
        void* item = NULL;
        int ret    = queue_.Take(&item, 1);
        if (ret != 0)
        {
            return NULL;
        }

        return item;
    }

    void* TryPop()
    {
        return Pop();
    }

private:
    CircleQueue queue_;
};

#else
class Queue
{
public:
    Queue()
    {
        assert(0 == pthread_mutex_init(&mutex_, NULL));
        assert(0 == pthread_cond_init(&cond_, NULL));
    }

    ~Queue()
    {
    }

    void Push(void* item)
    {
        SmartLock locker(mutex_);
        container_.push(item);

        pthread_cond_signal(&cond_);
    }

    void* Pop()
    {
        SmartLock locker(mutex_);
        while (container_.empty())
        {
            pthread_cond_wait(&cond_, &mutex_);
        }

        void* item = container_.front();
        container_.pop();
        return item;
    }

    void* TryPop()
    {
        if (container_.empty())
        {
            return NULL;
        }

        SmartLock locker(mutex_);
        if (container_.empty())
        {
            return NULL;
        }

        void* item = container_.front();
        container_.pop();
        return item;
    }

private:
    std::queue<void*> container_;
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;
};
#endif
