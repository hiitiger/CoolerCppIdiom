#pragma once


namespace Qt_
{
    void async(const std::function<void()>& cb);
    void asyncDelayed(const std::function<void()>& cb, int milliSecs);
}