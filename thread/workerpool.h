#pragma once

#include "syncqueue.h"

class WorkerPool
{
    std::vector<std::thread> threads_;
    SyncQueue<std::function<void()>> tasks_;
    std::mutex lock_;
    std::atomic<bool> running_ = false;

    WorkerPool(const WorkerPool&) = delete;
    WorkerPool& operator=(const WorkerPool&) = delete;

public:
    explicit WorkerPool(unsigned int size = std::thread::hardware_concurrency());
    ~WorkerPool();

    //default
    static WorkerPool* pool();
    static void async(const std::function<void()>& task);
    static void async(std::function<void()>&& task);

    void add(const std::function<void()>& task);
    void add(std::function<void()>&& task);

    void restart(unsigned int size);
    void stop();

protected:
    void start(unsigned int size);
    void threadRun();
};

inline WorkerPool::WorkerPool(unsigned int size /*= std::thread::hardware_concurrency()*/)
{
    start(size);
}

inline WorkerPool::~WorkerPool()
{
    stop();
}

//if build this as a dynamic library make sure put this function into cpp source file
inline WorkerPool* WorkerPool::pool()
{
    static WorkerPool pool;
    return &pool;
}

inline void WorkerPool::async(const std::function<void()>& task)
{
    pool()->add(task);
}

inline void WorkerPool::async(std::function<void()>&& task)
{
    pool()->add(move(task));
}

inline void WorkerPool::add(const std::function<void()>& task)
{
    tasks_.enqueue(task);
}

inline void WorkerPool::add(std::function<void()>&& task)
{
    tasks_.enqueue(move(task));
}

inline void WorkerPool::restart(unsigned int size)
{
    stop();
    start(size);
}

inline void WorkerPool::stop()
{
    std::lock_guard<std::mutex> lock(lock_);
    if (!running_)
    {
        return;
    }

    running_ = false;
    tasks_.stop();
    for (auto& thread : threads_)
    {
        thread.join();
    }
    threads_.clear();
}

inline void WorkerPool::start(unsigned int size)
{
    std::lock_guard<std::mutex> lock(lock_);
    if (running_)
    {
        return;
    }
    running_ = true;
    tasks_.start();
    for (unsigned int i = 0; i != size; ++i)
    {
        threads_.push_back(std::thread(&WorkerPool::threadRun, this));
    }
}

inline void WorkerPool::threadRun()
{
    while (true)
    {
        std::function<void()> func;
        if (tasks_.dequeue(func))
        {
            if (!running_)
            {
                break;
            }

            func();
        }

        if (!running_)
        {
            break;
        }
    }
}
