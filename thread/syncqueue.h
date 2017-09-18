#pragma once

template<class T>
class SyncQueue 
{
    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<T> queue_;
    bool quit_ = false;

    SyncQueue(const SyncQueue&) = delete;
    SyncQueue &operator=(const SyncQueue &) = delete;

public:
    SyncQueue() = default;
    ~SyncQueue();

    void restart();
    void stop();

    bool isEmpty();

    void enqueue(const T& item);
    void enqueue(T&& item);

    bool try_dequeue(T& item);
    bool try_dequeueAll(std::deque<T>& items);

    bool dequeue(T& item);
    bool dequeueAll(std::deque<T>& items);

};

template<class T> SyncQueue<T>::~SyncQueue()
{
    stop();
}

template<class T> void SyncQueue<T>::restart()
{
    std::unique_lock<std::mutex> lock(mutex_);
    quit_ = false;
}

template<class T> void SyncQueue<T>::stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    quit_ = true;
    cv_.notify_all();
}

template<class T> bool SyncQueue<T>::isEmpty()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.empty();
}

template<class T> void SyncQueue<T>::enqueue(const T& item)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push_back(item);
    }

    cv_.notify_one();
}

template<class T> void SyncQueue<T>::enqueue(T&& item)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push_back(std::move(item));
    }

    cv_.notify_one();
}

template<class T> bool SyncQueue<T>::try_dequeue(T& item)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty())
    {
        return false;
    }

    item = std::move(queue_.front());
    queue_.pop_front();
    return true;
}

template<class T> bool SyncQueue<T>::try_dequeueAll(std::deque<T>& items)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty())
    {
        return false;
    }

    items = std::move(queue_);
    return true;
}

template<class T> bool SyncQueue<T>::dequeue(T& item)
{
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_.empty() && !quit_)
    {
        cv_.wait(lock);
    }

    if (quit_)
    {
        return false;
    }

    item = std::move(queue_.front());
    queue_.pop_front();
    return true;
}

template<class T> bool SyncQueue<T>::dequeueAll(std::deque<T>& items)
{
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_.empty() && !quit_)
    {
        cv_.wait(lock);
    }

    if (quit_)
    {
        return false;
    }

    items = std::move(queue_);
    return true;
}