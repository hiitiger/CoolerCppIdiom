#pragma once


class ConsolePerfTimer
{
    const char* msg_;

    std::chrono::microseconds start_ = std::chrono::microseconds::zero();
public:
    ConsolePerfTimer(const char* msg)
        : msg_(msg)
    {
        using namespace std::chrono;
        auto tp = steady_clock::now();
        auto cur = tp.time_since_epoch();
        start_ = duration_cast<microseconds>(cur);
    }

    ~ConsolePerfTimer()
    {
        using namespace std::chrono;
        auto tp = steady_clock::now();
        auto cur = tp.time_since_epoch();
        auto end_ = duration_cast<microseconds>(cur);

        auto timeCost = (end_ - start_).count();
        std::cout << msg_ << " cost " << timeCost << " microseconds" << std::endl;
    }
};


#define  PRINT_TIME(msg) ConsolePerfTimer print_time_##msg(#msg)
