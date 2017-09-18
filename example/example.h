#pragma 
#include "../tool/snowflake.h"
#include "../tool/throttle.h"

void example_throttle()
{
    {
        Throttle throttle(3, 100);
        std::vector<bool> vec;
        for (auto i = 0; i != 10; i++)
        {
            vec.push_back(throttle.tick());
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
        std::vector<bool> vec_expect = { true, true, true, false, true, true, true, false, true, true };
        assert(vec_expect == vec);
    }

    {
        Throttle throttle(1, 300);
        std::vector<bool> vec;
        for (auto i = 0; i != 10; i++)
        {
            vec.push_back(throttle.tick());
            std::this_thread::sleep_for(std::chrono::milliseconds(101));
        }
        std::vector<bool> vec_expect = { true, false, false, true, false, false, true, false, false, true };
        assert(vec_expect == vec);
    }

    {
        Throttle throttle(1, 300);
        std::vector<bool> vec;
        for (auto i = 0; i != 10; i++)
        {
            vec.push_back(throttle.tick());
            std::this_thread::sleep_for(std::chrono::milliseconds(90));
        }
        std::vector<bool> vec_expect = { true, false, false, false, true, false, false, false, true, false };
        assert(vec_expect == vec);
    }
}


void example_snowflake()
{
    using namespace std::chrono;
    {
        Snowflake<true> ff;
        int64_t start = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
        priv::usleep((start / 1000 + 1) * 1000 - start);
        start = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
        for (auto i = 0; i != 4096 * 1; ++i)
        {
            ff.generate();
        }
        int64_t end = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();

        std::cout << "duration:" << end - start << std::endl;
    }

    {
        Snowflake<true> ff;
        int64_t start = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
        priv::usleep((start / 1000 + 1) * 1000 - start);
        start = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
        for (auto i = 0; i != 4096 * 2; ++i)
        {
            ff.generate();
        }
        int64_t end = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();

        std::cout << "duration:" << end - start << std::endl;
    }

}