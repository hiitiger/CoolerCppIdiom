#include "stable.h"
#include "qasync.h"
#include "time/timetick.h"

namespace
{
    typedef std::function<void()> Callback0;

    struct WrapTask
    {
    public:

        WrapTask(const Callback0& func, const TimeTick& runTime)
            : func_(func), run(runTime)
        {

        }

        WrapTask(Callback0&& func, const TimeTick& runTime)
            : func_(std::move(func)), run(runTime)
        {

        }

        WrapTask(const Callback0& func)
            : func_(func)
        {

        }

        WrapTask(Callback0&& func)
            : func_(func)
        {

        }

        WrapTask(WrapTask&&) = default;

        WrapTask& operator=(WrapTask&&) = default;

        void invoke()
        {
            func_();
        }

        bool operator < (const WrapTask& other) const
        {
            if (run < other.run)
            {
                return false;
            }
            if (run > other.run)
            {
                return true;
            }
            return seq_ > other.seq_;
        }


        Callback0 func_;

        TimeTick run;

        int seq_ = 0;

    };
}

class  QAppAsyncEvent : public QEvent
{
public:
    static const int k_qAppAsyncEventType;
    QAppAsyncEvent()
        : QEvent(QEvent::Type(k_qAppAsyncEventType))

    {
    }

};

const int QAppAsyncEvent::k_qAppAsyncEventType = QEvent::registerEventType();


class QxApplicationPrivate
{
    friend class QxApplication;
    QxApplication* q_ptr;
    std::atomic<int> taskSeq_ = 0;
    std::thread::id thread_id_;
public:
    QxApplicationPrivate(QxApplication* q) : q_ptr(q)
        , thread_id_(std::this_thread::get_id())
    {

    }

    void add(WrapTask&& task)
    {
        if (m_quit) { return; }

        std::lock_guard<std::mutex> lock(m_taskQueueLock);
        taskSeq_ += 1;
        task.seq_ = taskSeq_;
        m_taskQueue.push_back(std::move(task));
        QAppAsyncEvent* ev = new QAppAsyncEvent();
        qApp->postEvent(qApp, ev);
    }

    void post(Callback0&& task)
    {
        add(std::move(task));
    }

    void post(const Callback0& task)
    {
        add(task);
    }

    void postDelayed(Callback0&& task, int milliSeconds)
    {
        add(WrapTask(std::move(task), TimeTick::now().addMilliSecs(milliSeconds)));
    }

    void postDelayed(const Callback0& task, int milliSeconds)
    {
        add(WrapTask(task, TimeTick::now().addMilliSecs(milliSeconds)));
    }

    void runOnce()
    {
        runTaskQueue();
        runDelayQueue();
        if (!nextDelay_.isNull())
        {
            auto now = TimeTick::now();
            int toWait = qMax(1, (int)((nextDelay_ - now).milliSecs()));
            QTimer::singleShot(toWait, q_ptr, SLOT(runQueue()));
        }
    }

    void runTaskQueue()
    {
        std::deque<WrapTask> taskQueue;
        {
            std::lock_guard<std::mutex> lock(m_taskQueueLock);
            taskQueue = std::move(m_taskQueue);
        }
        auto now = TimeTick::now();

        for (auto it = taskQueue.begin(); it != taskQueue.end(); ++it)
        {
            if (it->run.isNull())
            {
                it->invoke();
            }
            else
            {
                if (now >= it->run)
                {
                    it->invoke();
                    now = TimeTick::now();
                }
                else
                {
                    m_delayQueue.push(std::move(*it));
                }
            }
        }
    }

    void runDelayQueue()
    {
        auto now = TimeTick::now();

        while (true)
        {
            if (m_delayQueue.empty() || m_delayQueue.top().run > now)
            {
                break;
            }
            WrapTask task = std::move(const_cast<WrapTask&>(m_delayQueue.top()));
            m_delayQueue.pop();
            task.invoke();
            now = TimeTick::now();
        }

        if (m_delayQueue.empty())
        {
            nextDelay_.setZero();
        }
        else
        {
            nextDelay_ = m_delayQueue.top().run;
        }
    }

    std::atomic<bool> m_quit = false;
    std::mutex m_taskQueueLock;
    std::deque<WrapTask> m_taskQueue;
    std::priority_queue<WrapTask> m_delayQueue;
    TimeTick nextDelay_;
};


QxApplicationPrivate* g_appPriv = nullptr;


QxApplication::QxApplication(int argc, char* argv[])
    : QCoreApplication(argc, argv)
    , d_ptr(new QxApplicationPrivate(this))
{
    g_appPriv = d_ptr;
}

QxApplication::~QxApplication()
{
    delete d_ptr;
}

QxApplication* QxApplication::instance()
{
    return qobject_cast<QxApplication*>(QCoreApplication::instance());
}


std::thread::id QxApplication::threadId()
{
    return d_ptr->thread_id_;
}

void QxApplication::runQueue()
{
    d_ptr->runOnce();
}

bool QxApplication::event(QEvent* ev)
{
    if (ev->type() == QAppAsyncEvent::k_qAppAsyncEventType)
    {
        runQueue();
        ev->accept();
        return true;
    }

    return __super::event(ev);
}

namespace Qx
{
    void Qx::async(const std::function<void()>& cb)
    {
        g_appPriv->post(cb);
    }

    void Qx::asyncDelayed(const std::function<void()>& cb, int milliSecs)
    {
        g_appPriv->postDelayed(cb, milliSecs);
    }

    std::thread::id threadId()
    {
        return QxApplication::instance()->threadId();
    }

}