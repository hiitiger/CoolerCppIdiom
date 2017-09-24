#pragma once

class FpsTimer
{
    std::chrono::microseconds lastTickTime_ = std::chrono::microseconds::zero();
    std::chrono::microseconds delta_ = std::chrono::microseconds::zero();
    std::chrono::microseconds lastTime_ = std::chrono::microseconds::zero();
    std::uint32_t fps_ = 0;
    std::uint32_t frameCount_ = 0;
public:
    FpsTimer();

    void start();

    void tick();

    double tickDelta();

    std::uint32_t fps();
};

inline FpsTimer::FpsTimer()
{
    start();
}

inline void FpsTimer::start()
{
    using namespace std::chrono;
    lastTickTime_ = lastTime_ = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch());
    frameCount_ = 0;
    fps_ = 0;
    delta_ = microseconds(0);
}

inline void FpsTimer::tick()
{
    frameCount_ += 1;
    using namespace std::chrono;
    microseconds currTime = duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch());
    if ((currTime - lastTime_).count() > 1000 * 1000)
    {
        fps_ = frameCount_;
        frameCount_ = 0;
        lastTime_ = currTime;
    }

    delta_ = currTime - lastTickTime_;
    lastTickTime_ = currTime;
}

inline double FpsTimer::tickDelta()
{
    return  (double)delta_.count() / 1000.0;
}

inline std::uint32_t FpsTimer::fps()
{
    return fps_;
}

