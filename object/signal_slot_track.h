#pragma once

namespace signal_slot
{
    template<class T>
    struct arg_traits
    {
        typedef const T& param_type;
    };

    template<class T>
    struct arg_traits<T&>
    {
        typedef T& param_type;
    };

    template<class T>
    struct arg_traits<const T&>
    {
        typedef const T& param_type;
    };

    template<class T>
    struct arg_traits<T&&>
    {
        typedef T&& param_type;
    };

    template<class T>
    using arg_traits_t = typename arg_traits<T> ::param_type;


    class dummy_lock
    {
    public:
        void lock() {}
        void unlock() {}
    };

    typedef dummy_lock st_policy;
    typedef std::mutex mt_policy;

    typedef mt_policy def_thread_policy;

    template<class thread_policy>
    class trackable;

    template <class, class>
    class signal;

    class connection_context_base;

    template<class T, class thread_policy>
    class connection_context;

    template<class thread_policy>
    class signal_base
    {
        signal_base(const signal_base &) = delete; 
        signal_base &operator=(const signal_base &) = delete;
    public:
        signal_base() = default;
        virtual ~signal_base() {}
        virtual void connectionDisconnect(connection_context_base*) = 0;
        virtual void trackableExpired(trackable<thread_policy>* object) = 0;
    };

    class connection_context_base
    {
    protected:
        std::atomic<bool> connected_ = true;
    public:
        connection_context_base() : connected_(true) {}
        virtual ~connection_context_base() {}

        virtual void disconnectSelf() = 0;
        virtual void trackableExpired() = 0;
        virtual void lock() = 0;
        virtual void unlock() = 0;

        bool isConnected() const { return connected_; }

        void disconnect()
        {
            //disconnect 
            //so this will require lock
            disconnectSelf();
        }

        void disconnected()
        {
            std::lock_guard<connection_context_base> lock(*this);
            connected_ = false;
        }

        void nolockDisconnected()
        {
            connected_ = false;
        }
    };


    template< class ...A, class thread_policy>
    class connection_context<void(A...), thread_policy> : public connection_context_base
    {
        signal_base<thread_policy>* signal_;
        std::function<void(A...)> slot_;
        trackable<thread_policy>* contextObject_;
        thread_policy lock_;
    public:
        connection_context(signal_base<thread_policy>* s, std::function<void(A...)>&& func, trackable<thread_policy>* object)
            : signal_(s), slot_(std::move(func)), contextObject_(object)
        {

        }

        void disconnectSelf() override
        {
            std::lock_guard<connection_context> lock(*this);
            if (isConnected())
            {
                nolockDisconnected();
                signal_->connectionDisconnect(this);
            }
        }

        void trackableExpired() override
        {
            std::lock_guard<connection_context> lock(*this);
            if (isConnected())
            {
                nolockDisconnected();
                signal_->trackableExpired(contextObject_);
            }
        }

        void lock() override
        {
            lock_.lock();
        }

        void unlock() override
        {
            lock_.unlock();
        }

        void invoke(A ...args)
        {
            // here we don't need lock
            if (isConnected())
            {
                slot_(std::forward<A>(args)...);
            }
        }

        trackable<thread_policy>* getTrackable()
        {
            std::lock_guard<connection_context> lock(*this);
            return contextObject_;
        }

        trackable<thread_policy>* nolockGetTrackable() const
        {
            return contextObject_;
        }
    };

    class connection
    {
    protected:
        std::weak_ptr<connection_context_base> context_;
    public:
        connection(const std::weak_ptr<connection_context_base>& d)
            :context_(std::weak_ptr<connection_context_base>(d))
        {

        }

        void disconnect()
        {
            if (auto context = context_.lock())
            {
                context->disconnect();
            }
        }

    };

    template<class thread_policy = def_thread_policy>
    class trackable
    {
        trackable(const trackable &) = delete;
        trackable &operator=(const trackable &) = delete;

        thread_policy trackableLock_;
        std::set<std::shared_ptr<connection_context_base>> connections_;
    public:
        trackable() = default;
        virtual ~trackable()
        {
            removeAll();
        }

        void connectionAdded(const std::shared_ptr<connection_context_base> & conn)
        {
            std::lock_guard<thread_policy> lock(trackableLock_);
            connections_.insert(conn);
        }

        void connectionRemoved(const std::shared_ptr<connection_context_base> & conn)
        {
            std::lock_guard<thread_policy> lock(trackableLock_);
            auto it = connections_.find(conn);
            if (it != connections_.end())
            {
                connections_.erase(it);
            }
        }

        void removeAll()
        {
            std::lock_guard<thread_policy> lock(trackableLock_);
            for (auto it = connections_.begin(); it != connections_.end(); ++it)
            {
                (*it)->trackableExpired();
            }
            connections_.clear();
        }

    };

    template <class ...A, class thread_policy>
    class signal<void(A ...), thread_policy> : public signal_base<thread_policy>
    {
        using connection_context = connection_context<void(A ...), thread_policy>;
        std::shared_ptr<std::vector<std::shared_ptr<connection_context>>> connections_;
        thread_policy lock_;

        void compactConnections()
        {
            //make sure operation on connections is safe
            if (!connections_.unique())
            {
                connections_.reset(new std::vector<std::shared_ptr<connection_context>>(*connections_));
            }
        }

        connection add(const std::shared_ptr<connection_context>& conn, trackable<thread_policy>* trackable = nullptr)
        {
            compactConnections();

            connections_->emplace_back(conn);
            if (trackable)
            {
                trackable->connectionAdded(conn);
            }
            return connection(conn);
        }

        void connectionDisconnect(connection_context_base* conn) override
        {
            std::lock_guard<thread_policy> lock(lock_);
            compactConnections();

            auto it = connections_->begin();
            for (; it != connections_->end();)
            {
                if ((*it).get() == conn)
                {
                    auto conn = *it;
                    auto object = conn->nolockGetTrackable();
                    connections_->erase(it);
                    if (object)
                    {
                        object->connectionRemoved(conn);
                    }
                    return;
                }
                else
                {
                    ++it;
                }
            }
        }

        void trackableExpired(trackable<thread_policy>* object) override
        {
            std::lock_guard<thread_policy> lock(lock_);
            compactConnections();

            auto it = connections_->begin();
            for (; it != connections_->end();)
            {
                if ((*it)->getTrackable() == object)
                {
                    auto conn = *it;
                    it = connections_->erase(it);
                    continue;
                }
                else
                {
                    ++it;
                }
            }
        }

    public:
        signal() : connections_(std::make_shared<std::vector<std::shared_ptr<connection_context>>>())
        {

        }

        ~signal()
        {
            removeAll();
        }

        connection connect(std::function<void(A...)>&& slot)
        {
            std::lock_guard<thread_policy> lock(lock_);
            return add(std::make_shared<connection_context>(this, std::move(slot), nullptr));
        }

        connection addTrackable(std::function<void(A...)>&& slot, trackable<thread_policy>* object)
        {
            std::lock_guard<thread_policy> lock(lock_);
            return add(std::make_shared<connection_context>(this, std::move(slot), object), object);
        }

        void removeTrackable(trackable<thread_policy>* object)
        {
            std::lock_guard<thread_policy> lock(lock_);
            compactConnections();

            auto it = connections_->begin();
            for (; it != connections_->end();)
            {
                if ((*it)->getTrackable() == object)
                {
                    auto conn = *it;
                    conn->disconnected();
                    it = connections_->erase(it);
                    object->connectionRemoved(conn);
                    continue;
                }
                else
                {
                    ++it;
                }
            }
        }

        void removeAll()
        {
            std::lock_guard<thread_policy> lock(lock_);

            for (auto it = connections_->begin(); it != connections_->end(); ++it)
            {
                auto conn = *it;
                conn->disconnected();
                auto object = (*it)->getTrackable();
                if (object)
                {
                    object->connectionRemoved(conn);
                }
            }
            connections_->clear();
        }

        void emit(arg_traits_t<A>...args)
        {
            //so we don't need recursive lock
            std::shared_ptr<std::vector<std::shared_ptr<connection_context>>> connections;
            {
                std::lock_guard<thread_policy> lock(lock_);
                connections = connections_;
            }
            for (auto conn : *connections)
            {
                conn->invoke(arg_traits_t<A>(args)...);
            }
        }

        void operator()(A ...args)
        {
            return (*this).emit(std::forward<A>(args)...);
        }
    };

    template<class S, class thread_policy = def_thread_policy>
    class signal : public signal<S, thread_policy>
    {
    };

}
