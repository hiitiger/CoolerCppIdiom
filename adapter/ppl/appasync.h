#pragma once
#include <ppltasks.h>

namespace AppUI
{
    /*
        implement this depending on ui framework
    */
    void async(const std::function<void()>& cb);
    void async(std::function<void()>&& cb);
    void asyncDelayed(const std::function<void()>& cb, int milliSecs);
    void asyncDelayed(std::function<void()>&& cb, int milliSecs);

    std::shared_ptr<concurrency::scheduler_interface> static_pplScheduler();
    std::thread::id threadId();

}


namespace concurrency_
{
    std::shared_ptr<concurrency::scheduler_interface>& static_pplScheduler();

    template<class T>
    class task_completion_event_once
    {
        task_completion_event_once(const task_completion_event_once &) = delete;
        task_completion_event_once(task_completion_event_once &&) = delete;
        task_completion_event_once&operator=(const task_completion_event_once &) = delete;
        task_completion_event_once&operator=(task_completion_event_once &&) = delete;
    public:
        concurrency::task_completion_event<T> tce;
        task_completion_event_once() = default;

        bool set(T _Result)
        {
            bool ret = tce.set(_Result);
            tce = concurrency::task_completion_event<T>();
            return ret;
        }
    };

    template<>
    class task_completion_event_once<void>
    {
        task_completion_event_once(const task_completion_event_once &) = delete;
        task_completion_event_once(task_completion_event_once &&) = delete;
        task_completion_event_once&operator=(const task_completion_event_once &) = delete;
        task_completion_event_once&operator=(task_completion_event_once &&) = delete;
    public:
        concurrency::task_completion_event<void> tce;
        task_completion_event_once() = default;

        bool set()
        {
            bool ret = tce.set();
            tce = concurrency::task_completion_event<void>();
            return ret;
        }
    };


    template<typename _Ty>
    __forceinline auto async(_Ty _Param)
    {
        return concurrency::create_task(_Param, concurrency::task_options(concurrency_::static_pplScheduler()));
    }

    template<typename _Ty>
    __forceinline auto asyncOnUI(_Ty _Param)
    {
        return concurrency::create_task(_Param, concurrency::task_options(AppUI::static_pplScheduler()));
    }

    template<typename T>
    __forceinline auto asyncWait(const concurrency::task_completion_event<T>& _Event)
    {
        return concurrency::create_task(_Event, concurrency::task_options(concurrency_::static_pplScheduler()));
    }

    template<typename T>
    __forceinline auto asyncWait(const concurrency_::task_completion_event_once<T>& _Event)
    {
        return concurrency::create_task(_Event.tce, concurrency::task_options(concurrency_::static_pplScheduler()));
    }

    __forceinline void setup_ambient_scheduler()
    {
        concurrency::set_ambient_scheduler(static_pplScheduler());
    }

    struct  FixAmbientScheduler
    {
        FixAmbientScheduler()
        {
            setup_ambient_scheduler();
        }
    };

    template<class F>
    struct PoolThenHolder
    {
        F _Func;
    };

    template<class F>
    struct UIThenHolder
    {
        F _Func;
    };

    template<class T, class F >
    __forceinline auto operator | (const concurrency::task<T>& _Task, PoolThenHolder<F>&& h)
    {
        return _Task.then(h._Func, concurrency::task_options(concurrency_::static_pplScheduler()));
    }

    template<class T, class F >
    __forceinline auto operator | (const concurrency::task<T>& _Task, UIThenHolder<F>&& h)
    {
        return _Task.then(h._Func, concurrency::task_options(AppUI::static_pplScheduler()));
    }

    template<class F>
    __forceinline auto _Pool(const F& _Func)
    {
        return PoolThenHolder<F>{_Func};
    }

    template<class F>
    __forceinline auto _UI(const F& _Func)
    {
        return UIThenHolder<F>{_Func};
    }

    struct Pool_
    {
        template<class F>
        __forceinline auto operator()(const F& _Func) const
        {
            return _Pool(_Func);
        }
    };

    struct UI_
    {
        template<class F>
        __forceinline auto operator()(const F& _Func) const
        {
            return _UI(_Func);
        }
    };

    template<class R>
    struct DelayThen
    {
        int _Timeout;
        __forceinline R operator()(R _Ret) const
        {
            return _Ret;
        }
    };

    template<>
    struct DelayThen<void>
    {
        int _Timeout;
        __forceinline void operator()(void) const
        {
        }
    };

    template<class R>
    __forceinline DelayThen<R> delay(int _Timeout)
    {
        return DelayThen<R>{_Timeout};
    }

    __forceinline DelayThen<void> delay(int _Timeout)
    {
        return DelayThen<void>{_Timeout};
    }

    template<class T, class R>
    __forceinline auto operator | (const concurrency::task<T>& _Task, DelayThen<R>&& d)
    {
        auto _Then = [d = std::move(d)](R _Ret) {
            concurrency::task_completion_event<R> tce;
            AppUI::asyncDelayed([tce, d, _Ret]() {
                tce.set(_Ret);
            }, d._Timeout);

            concurrency::task<R> event_set(tce);
            return event_set.then(d, concurrency::task_options(concurrency_::static_pplScheduler()));
        };

        return _Task.then(_Then, concurrency::task_options(concurrency_::static_pplScheduler()));
    }

    template<class T, typename = std::enable_if_t<std::is_same<T, void>::value || std::is_same<T, concurrency::task<void>>::value, void>>
    __forceinline auto operator | (const concurrency::task<T>& _Task, DelayThen<void>&& d)
    {
        auto _Then = [d = std::move(d)]() {
            concurrency::task_completion_event<void> tce;
            AppUI::asyncDelayed([tce, d]() {
                tce.set();
            }, d._Timeout);

            concurrency::task<T> event_set(tce);
            return event_set.then(d, concurrency::task_options(concurrency_::static_pplScheduler()));
        };

        return _Task.then(_Then, concurrency::task_options(concurrency_::static_pplScheduler()));
    }

    inline auto delayed(int _Timeout)
    {
        concurrency::task_completion_event<void> tce;
        AppUI::asyncDelayed([tce]() {
            tce.set();
        }, _Timeout);

        concurrency::task<void> event_set(tce);
        return event_set.then([]() {}, concurrency::task_options(concurrency_::static_pplScheduler()));
    }


    namespace
    {
        static const concurrency_::FixAmbientScheduler fix_ppl_ambient_scheduler = concurrency_::FixAmbientScheduler();

        static const auto pool = concurrency_::Pool_();
        static const auto ui = concurrency_::UI_();
    }

    ///---------------
    ///---------------   
    template<class _Ty>
    struct Awaiter
    {
        concurrency::task<_Ty> _Task;

        std::thread::id thread_id = std::this_thread::get_id();

        Awaiter(concurrency::task<_Ty>&& t) : _Task(std::move(t))
        {

        }

        Awaiter(concurrency::task<_Ty>&& t, std::thread::id th_id) : _Task(std::move(t)), thread_id(th_id)
        {

        }

        bool await_ready()
        {
            return _Task.is_done();
        }

        template <typename _Handle>
        void await_suspend(_Handle _ResumeCb)
        {
            if (thread_id == AppUI::threadId())
            {
                auto op = concurrency::task_options(AppUI::static_pplScheduler());
                op.set_continuation_context(concurrency::task_continuation_context::get_current_winrt_context());
                _Task.then([_ResumeCb](concurrency::task<_Ty>&)
                {
                    _ResumeCb();
                }, op);
            }
            else
            {
                auto op = concurrency::task_options(concurrency_::static_pplScheduler());
                op.set_continuation_context(concurrency::task_continuation_context::get_current_winrt_context());
                _Task.then([_ResumeCb](concurrency::task<_Ty>&)
                {
                    _ResumeCb();
                }, op);
            }
        }
        auto await_resume()
        {
            return _Task.get();
        }
    };

    template<class _Ty>
    struct AwaiterUI : public Awaiter<_Ty>
    {
        AwaiterUI(concurrency::task<_Ty>&& t)
            : Awaiter(std::move(t), AppUI::threadId())
        {

        }
    };

    template<class _Ty>
    struct AwaiterPool : public Awaiter<_Ty>
    {
        AwaiterPool(concurrency::task<_Ty>&& t)
            : Awaiter(std::move(t), std::thread::id())
        {

        }
    };
}

template <typename _Ty>
auto operator co_await(concurrency::task<_Ty> &&f)
{
    return concurrency_::Awaiter<_Ty>{std::move(f)};
}

#define  __await co_await
#define  __yield co_yield
#define  __return co_return

#define  __awaitui co_await( concurrency_::AwaiterUI<void>(concurrency_::async([]{})))
#define  __awaitpool co_await( concurrency_::AwaiterPool<void>(concurrency_::async([]{})))