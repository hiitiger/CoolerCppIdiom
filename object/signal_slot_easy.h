#pragma once

namespace signal_slot_esay
{
    class connection_context_base
    {
        std::atomic<bool>  connected_ = true;
    public:
        virtual ~connection_context_base(){}
        virtual void disconnectSelf() = 0;

        bool isConnected()
        {
            return connected_;
        }

        void disconnect()
        {
            disconnectSelf();
            connected_ = false;
        }

        void disconnected()
        {
            connected_ = false;
        }

    };

    class signal_base
    {
    public:
        virtual ~signal_base(){}
        virtual void removeConnection(connection_context_base* conn) = 0;
    };

    template<class>
    class signal;

    template<class >
    class connection_context;

    template<class ...A>
    class connection_context<void(A...)> : public connection_context_base
    {
        std::function<void(A...)> slot_fun_;
        signal_base* signal_;
    public:
        connection_context(signal_base* signal, std::function<void(A...)>&& slot_fun)
            : signal_(signal), slot_fun_(std::move(slot_fun))
        {
            
        }

        virtual void disconnectSelf()
        {
            signal_->removeConnection(this);
        }

        void invoke(A... args)
        {
            if (isConnected())
            {
                slot_fun_(std::forward<A>(args)...);
            }
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

    
    template<class ...A>
    class signal<void(A...)> : public signal_base
    {
        std::list<std::shared_ptr<connection_context<void(A...)>>> connections_;

        connection add(const std::shared_ptr<connection_context<void(A...)>>& conn)
        {
            connections_.emplace_back(conn);
            return connection(conn);
        }

        void removeConnection(connection_context_base* conn) override
        {
            auto it = connections_.begin();
            for (; it != connections_.end();)
            {
                if ((*it).get() == conn)
                {
                    auto conn = *it;
                    conn->disconnected();
                    connections_.erase(it);
                    return;
                }
                else
                {
                    ++it;
                }
            }
        }
    public:
        signal() = default;

        connection connect(std::function<void(A...)>&& slot_func)
        {
            return add(std::make_shared<connection_context<void(A...)>>(this, std::move(slot_func)));
        }

        void operator()(A... args)
        {
            auto it = connections_.begin();
            for (; it != connections_.end();)
            {
                auto next = it;
                ++next;
                (*it)->invoke(std::forward<A>(args)...);
                it = next;
            }
        }

        void emit(A... args)
        {
            (*this)(std::forward<A>(args)...);
        }
    };
}

