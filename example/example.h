#pragma 
#include "../tool/snowflake.h"
#include "../tool/throttle.h"
#include "../object/signal_slot_easy.h"
#include "../object/signal_slot_track.h"
#include "../time/datetime.h"
#include "../thread/workerpool.h"
#include "../object/comptr.h"
#include "../object/copyonwrite.h"
#include "../container/skiplist.h"
#include "../tool/utils.h"
#include "../tool/utlils_num.h"

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

    WorkerPool pool;
    Snowflake<true> ff;
    for (int i = 0; i != 10; ++i)
    {
        pool.add([&]() {
            int64_t start = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
            priv::usleep((start / 1000 + 1) * 1000 - start);
            start = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
            for (auto i = 0; i != 4096 * 1; ++i)
            {
                ff.generate();
            }
            int64_t end = duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();

            std::cout << std::this_thread::get_id() << ", duration:" << end - start << std::endl;
        });
    }
    std::this_thread::sleep_for(seconds(1));
}


void example_signal_slot()
{
    using namespace signal_slot;
    bool slot2 = false;
    bool slot1 = false;
    signal<void(const std::string&, std::string&&)> signal1;

    connection conn1 = signal1.connect([&slot1](const std::string v, std::string&& str){
        slot1 = true;
        std::cout << "slot 1: " << v << std::string(std::move(str)) << std::endl;
    });

    connection conn2 = signal1.connect([&slot2, &conn2](const std::string v, std::string&& str) {
        slot2 = true;
        std::cout << "slot 2: " << v << str << std::endl;
        conn2.disconnect();
    });
    std::string value = "value";
   // const std::string str = ;
    signal1.emit(value, ("jijiji"));
    assert(slot1);
    assert(slot2);

    slot2 = slot1 = false;
 //   conn2.disconnect();
    signal1(value, std::string("jiujiujiu"));
    assert(slot1);
    assert(!slot2);
}


void example_datetime()
{
    auto now = DateTime::now();
    std::cout << now << std::endl;
}


void exmaple_workerpool()
{
    WorkerPool pool;
    std::mutex lock;
    std::map<int, std::thread::id> vec;
    std::set<std::thread::id> tvec;
    std::atomic<int> sz = 0;
    for (auto i = 0; i != 100; ++i)
    {
        pool.add([i, &vec, &tvec, &sz, &lock]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::lock_guard<std::mutex> scopeLock(lock);
            vec.insert(std::make_pair(i, std::this_thread::get_id()));
            tvec.insert(std::this_thread::get_id());
            sz += 1;
        });
    }
    while(sz != 100)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::cout << vec.size() <<"\t" << tvec.size()<<std::endl;
}

void exmaple_strings()
{
    auto res_l = utils::to_lower<std::string>("aBc");
    auto res_u = utils::to_upper<std::string>("aBc");
    assert(res_l == "abc");
    assert(res_u == "ABC");

    std::vector<std::string> rr;
    auto cvec = utils::split<std::string>(("123, fa,143,,  ,,"), ',', false);
    assert(cvec == std::vector<std::string>({ "123", " fa", "143", "", "  ", "", "" }));
    auto cvec2 = utils::split<std::string>(("123, fa,143,,  ,,"), ',');
    assert(cvec2 == std::vector<std::string>({ "123", " fa", "143",  "  " }));

    auto rsstr = utils::replace_substr<std::wstring>(L"123@1@123@1@1avc@1", L"@1", L"@2");
    assert(utils::ends_with(rsstr, L"@2"));

    std::vector<std::string> vec{ "1", "2", "3", "@", "abc", "." "com" };
    auto jvec = utils::join(vec, "fxxk");

    std::string jvec2;
    for (auto s : vec)
    {
        jvec2.append(s);
        jvec2.append("fxxk");
    }
    jvec2 = jvec2.substr(0, jvec2.size() - 4);

    assert(jvec == jvec2);
}