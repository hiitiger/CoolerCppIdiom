#pragma once
#include <assert.h>
#include <mutex>
#include <memory>
#include <atomic>
#include <set>
#include <vector>

namespace Storm 
{

    template<class>         class Callback;
    template<class T>       using Delegate = Callback<T>;

}

namespace Storm
{
    template <class T>
    struct identity
    {
        typedef T type;
    };

    template <class T>
    struct function_signature_impl
        : function_signature_impl<decltype(&T::operator())>
    {
    };

    template <class R, class ...A>
    struct function_signature_impl<R(A ...)>
        : identity<R(A ...)>
    {
        typedef R return_type;
    };

    template <class R, class ...A>
    struct function_signature_impl<R(*)(A ...)>
        : function_signature_impl<R(A ...)>
    {
    };

    template <class C, class R, class ...A>
    struct function_signature_impl<R(C::*)(A ...)>
        : function_signature_impl<R(A ...)>
    {
    };

    template <class C, class R, class ...A>
    struct function_signature_impl<R(C::*)(A ...) const>
        : function_signature_impl<R(A ...)>
    {
    };

    template <typename T>
    struct function_signature
        : function_signature_impl<T>
    {
    };

    template <class S>
    struct function_traits_args;

    template <class R, class ...A>
    struct function_traits_args<R(A ...)>
    {
        typedef R return_type;
        typedef std::tuple<A...> args_list;
    };

    template <int N, class T>
    struct args_traits_tuple_drop;

    template <>
    struct args_traits_tuple_drop<0, std::tuple<>>
    {
        typedef std::tuple<> type;
    };

    template <class A1, class ...A>
    struct args_traits_tuple_drop<0, std::tuple<A1, A...>>
    {
        typedef std::tuple<A1, A...> type;
    };

    template <int N, class A1, class ...A>
    struct args_traits_tuple_drop<N, std::tuple<A1, A...>> : public args_traits_tuple_drop<N - 1, std::tuple<A...>>
    {
    };

    template <class R, class A>
    struct function_traits_make_sig;

    template <class R, class ...A>
    struct function_traits_make_sig<R, std::tuple<A...>>
    {
        typedef R type(A ...);
    };

    template <class Functor>
    struct function_traits
    {
        using run_type = typename function_signature<typename std::decay<Functor>::type>::type;
        using return_type = typename function_signature<typename std::decay<Functor>::type>::return_type;
        static constexpr bool is_method = false;

    };

    template <class R, class ...A>
    struct function_traits<R(*)(A ...)>
    {
        using run_type = R(A ...);
        using return_type = R;
        static constexpr bool is_method = false;
    };

    template <class R, class C, class ...A>
    struct function_traits<R(C::*)(A ...)>
    {
        using run_type = R(C*, A ...);
        using return_type = R;
        static constexpr bool is_method = true;
    };

    template <class R, class C, class ...A>
    struct function_traits<R(C::*)(A ...) const>
    {
        using run_type = R(const C*, A ...);
        using return_type = R;
        static constexpr bool is_method = true;
    };

    template <class Fn, class ...A>
    struct funcion_traits_unbound_runtype
    {
        using fn_run_type = typename function_traits<typename std::decay<Fn>::type>::run_type;
        using args_type = typename function_traits_args<fn_run_type>::args_list;
        using unbound_args = typename args_traits_tuple_drop<sizeof...(A), args_type>::type;
        using unbound_runtype = typename function_traits_make_sig<typename function_traits_args<fn_run_type>::return_type, unbound_args>::type;
    };



    template<class T>
    using function_signature_t = typename function_signature<T>::type;

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

}

namespace Storm
{
    namespace helper
    {
        namespace CHECK
        {
            struct No
            {
            };

            template <typename T, typename Arg>
            No operator==(const T&, const Arg&);

            template <typename T, typename Arg = T>
            struct EqualExists
            {
                enum { value = !std::is_same<decltype(*(T*)(nullptr) == *(Arg*)(nullptr)), No>::value };
            };
        }

        template <class Fn, bool>
        struct IsSame;

        template <class T>
        struct IsSame<T, false>
        {
            static constexpr bool check(const T&, const T&)
            {
                return false;
            }
        };

        template <class T>
        struct IsSame<T, true>
        {
            static constexpr bool check(const T& a, const T& b)
            {
                return a == b;
            }
        };
    }

    struct BinderBase
    {
    public:
        BinderBase()
        {
        }

        virtual ~BinderBase()
        {
        }

        typedef void(*invoker_fun_type)();

        invoker_fun_type invoker_fun_ = nullptr;

        virtual bool isSameCallee(BinderBase* other) = 0;
    };

    template <bool is_mem_fun, class Fn, class ...LeftArgs>
    struct Binder;

    template <class Fn, class ...LeftArgs>
    struct Binder<false, Fn, LeftArgs...> : public BinderBase
    {
        Fn function_;
        std::tuple<LeftArgs...> left_args_;

        template <class F, class ...A>
        Binder(invoker_fun_type invoker, F&& function, A&&... args)
            : function_(std::forward<F>(function))
            , left_args_(std::forward<A>(args)...)
        {
            invoker_fun_ = invoker;
        }

        bool isSameCallee(BinderBase* other) override
        {
            if (Binder* otherOne = dynamic_cast<Binder*>(other))
            {
                return helper::IsSame<Fn, helper::CHECK::EqualExists<Fn>::value>::check(function_, otherOne->function_);
            }
            return false;
        }
    };

    template <class, class>
    struct MethodInvokerTraits;

    /* template<class C, class R>
    struct MethodInvokerTraits<C, R>
    {
    template<class MFn,  class ...A>
    R invoke(C&& object, MFn&& function, A&&... args)
    {
    return ((&object)->*function)(std::forward<A>(args)...);
    }
    };*/

    template <class C, class R>
    struct MethodInvokerTraits<C*, R>
    {
        template <class MFn, class ...A>
        static R invoke(C* object, MFn&& function, A&&... args)
        {
            return ((object)->*function)(std::forward<A>(args)...);
        }
    };

    template <class C, class R>
    struct MethodInvokerTraits<std::shared_ptr<C>, R>
    {
        template <class MFn, class ...A>
        static R invoke(std::shared_ptr<C>& object, MFn&& function, A&&... args)
        {
            return ((object.get())->*function)(std::forward<A>(args)...);
        }
    };

    template <class C, class R>
    struct MethodInvokerTraits<std::weak_ptr<C>, R>
    {
        template <class MFn, class ...A>
        static void invoke(std::weak_ptr<C>& object, MFn&& function, A&&... args)
        {
            if (auto obj_share_ptr = object.lock())
            {
                return ((obj_share_ptr.get())->*function)(std::forward<A>(args)...);
            }
        }
    };

    //template <class C, class R>
    //struct MethodInvokerTraits<RefPtr<C>, R>
    //{
    //    template <class MFn, class ...A>
    //    static R invoke(RefPtr<C>& object, MFn&& function, A&&... args)
    //    {
    //        return ((object.get())->*function)(std::forward<A>(args)...);
    //    }
    //};

    //template <class C, class R>
    //struct MethodInvokerTraits<WeakRefPtr<C>, R>
    //{
    //    template <class MFn, class ...A>
    //    static void invoke(WeakRefPtr<C>& object, MFn&& function, A&&... args)
    //    {
    //        if (auto obj_share_ptr = object.lock())
    //        {
    //            return ((obj_share_ptr.get())->*function)(std::forward<A>(args)...);
    //        }
    //    }
    //};


    template<class T, size_t sz>
    struct BinderObjectCheck
    {
        static constexpr bool check(T& t, T & other)
        {
            using CP = std::remove_reference_t<decltype(std::get<0>(t))>;
            using C = std::remove_reference_t<decltype(std::addressof(*(CP)0))>;
            return helper::IsSame<C, helper::CHECK::EqualExists<C>::value>::check(std::addressof(*std::get<0>(t)), std::addressof(*std::get<0>(other)));
        }
    };

    template<class T>
    struct BinderObjectCheck<T, 0>
    {
        static constexpr bool check(T&, T &)
        {
            return false;
        }
    };

    template <class MFn, class ...LeftArgs>
    struct Binder<true, MFn, LeftArgs...> : public BinderBase
    {
        MFn function_;
        std::tuple<LeftArgs...> left_args_;

        template <class F, class ...A>
        Binder(invoker_fun_type invoker, F&& function, A&&... args)
            : function_(std::forward<F>(function))
            , left_args_(std::forward<A>(args)...)
        {
            invoker_fun_ = invoker;
        }

        bool isSameCallee(BinderBase* other) override
        {
            if (Binder* otherOne = dynamic_cast<Binder*>(other))
            {
                static constexpr size_t num_bound_args = std::tuple_size<decltype(left_args_)>::value;

                return helper::IsSame<MFn, helper::CHECK::EqualExists<MFn>::value>::check(
                    function_, otherOne->function_)
                    && BinderObjectCheck<std::tuple<LeftArgs...>, num_bound_args>::check(left_args_, otherOne->left_args_);
            }
            return false;
        }

    };

    template <bool is_mem_fun, class B, class Fn>
    struct Invoker;

    template <class B, class R, class ...A>
    struct Invoker<false, B, R(A ...)>
    {
        static R invoke(BinderBase* binderBase, A ... args)
        {
            B* binder = static_cast<B*>(binderBase);
            static constexpr size_t num_bound_args = std::tuple_size<decltype(binder->left_args_)>::value;
            return apply(binder->function_, binder->left_args_, std::make_index_sequence<num_bound_args>{},
                std::forward<A>(args)...);
        }

    private:
        template <class Fn, class LeftArgs, std::size_t... Is>
        static R apply(Fn&& function, LeftArgs&& left_args, std::index_sequence<Is...>, A ... args)
        {
            (void)left_args;
            return function(std::get<Is>(std::forward<LeftArgs>(left_args))..., std::forward<A>(args)...);
        }
    };

    template <class B, class R, class ...A>
    struct Invoker<true, B, R(A ...)>
    {
        static R invoke(BinderBase* binderBase, A ... args)
        {
            B* binder = static_cast<B*>(binderBase);

            static constexpr size_t num_bound_args = std::tuple_size<decltype(binder->left_args_)>::value;

            return apply(binder->function_, binder->left_args_, std::make_index_sequence<num_bound_args>{},
                std::forward<A>(args)...);
        }

    private:
        template <class MFn, class LeftArgs, std::size_t... Is>
        static R apply(MFn&& function, LeftArgs&& left_args, std::index_sequence<Is...>, A ... args)
        {
            (void)left_args;
            return apply_(std::forward<MFn>(function), std::get<Is>(std::forward<LeftArgs>(left_args))..., std::forward<A>(args)...);
        }

        template<class MFn, class C, class ...Args>
        static R apply_(MFn&& function, C&& object, Args&&... args)
        {
            return MethodInvokerTraits<std::remove_reference_t<C>, R>::invoke(object, std::forward<MFn>(function), std::forward<Args>(args)...);
        }
    };

    template <class Fn, class R, class ...A, typename = std::enable_if_t<std::is_same<function_signature_t<std::decay_t<Fn>>, R(A ...)>::value, bool>>
    BinderBase* bindFunctor0_(Fn&& functor)
    {
        using UnboundRunType = R(A ...);
        using Binder = Binder<false, std::decay_t<Fn>>;
        using Invoker = Invoker<false, Binder, UnboundRunType>;
        using invoker_fun_type = BinderBase::invoker_fun_type;
        return static_cast<BinderBase*>(new Binder(reinterpret_cast<invoker_fun_type>(&Invoker::invoke), std::forward<Fn>(functor)));
    }

    template <class R, class ...A>
    BinderBase* bindStatic0_(R(*function)(A ...))
    {
        using Fn = R(*)(A ...);
        using UnboundRunType = R(A ...);
        using Binder = Binder<false, std::decay_t<Fn>>;
        using Invoker = Invoker<false, Binder, UnboundRunType>;
        using invoker_fun_type = BinderBase::invoker_fun_type;
        return static_cast<BinderBase*>(new Binder(reinterpret_cast<invoker_fun_type>(&Invoker::invoke), function));
    }

    class CBChecker
    {
    public:
        CBChecker()
        {
        }

        virtual ~CBChecker()
        {
        }
    };

    template <class Fn>
    class Callback;

    template <class R, class ...A>
    class Callback<R(A ...)> : public CBChecker
    {
        std::shared_ptr<BinderBase> binder_;
        typedef R(*invoker_type)(BinderBase*, A ...);

    public:
        Callback() = default;

        explicit Callback(std::shared_ptr<BinderBase>&& binder)
            : binder_(std::move(binder))
        {
        }

        Callback(const Callback& other) = default;

        Callback(Callback&& other) = default;

        Callback& operator=(const Callback& other) = default;

        Callback& operator=(Callback&& other) = default;

        template <class Fn, typename = std::enable_if_t<!std::is_same<std::remove_cv_t<std::remove_reference_t<Fn>>, Callback>::value, bool>>
        Callback(Fn&& functor)
        {
            binder_.reset(bindFunctor0_<Fn, R, A...>(std::forward<Fn>(functor)));
        }

        Callback(R(*function)(A ...))
        {
            binder_.reset(bindStatic0_<R, A...>(function));
        }

        bool operator==(const Callback& other) const
        {
            return binder_ == other.binder_;
        }

        bool operator !=(const Callback& other) const
        {
            return !operator==(other);
        }

        R operator()(A ... args) const
        {
            assert(!isEmpty());
            invoker_type invoker = reinterpret_cast<invoker_type>(binder_->invoker_fun_);
            return invoker(binder_.get(), std::forward<A>(args)...);
        }

        bool isEmpty() const
        {
            return binder_.get() == nullptr;
        }

        bool isSameCallee(const Callback& other) const
        {
            if (binder_)
            {
                return binder_->isSameCallee(other.binder_.get());
            }
            return false;
        }
    };

    template <class Fn, class ...A>
    Callback<typename funcion_traits_unbound_runtype<Fn, A...>::unbound_runtype> bind(Fn&& function, A&&... args)
    {
        static constexpr bool is_method = std::is_member_function_pointer<std::decay_t<Fn>>::value;
        using Binder = Binder<is_method, std::decay_t<Fn>, std::decay_t<A>...>;
        using UnboundRunType = typename funcion_traits_unbound_runtype<Fn, A...>::unbound_runtype;
        using Invoker = Invoker<is_method, Binder, UnboundRunType>;
        using Callback = Callback<UnboundRunType>;
        using invoker_fun_type = BinderBase::invoker_fun_type;
        return Callback(std::shared_ptr<BinderBase>(std::make_shared<Binder>(reinterpret_cast<invoker_fun_type>(&Invoker::invoke),
            std::forward<Fn>(function), std::forward<A>(args)...)));
    }

    template <typename Fun>
    struct is_fun_ptr
        : std::integral_constant<bool, std::is_pointer<Fun>::value && std::is_function<std::remove_pointer_t<Fun>>::value>
    {
    };

    //just to make it clear
    //enable move only lambda!!
    template <class Fn, class ...A>
    Callback<typename funcion_traits_unbound_runtype<Fn>::unbound_runtype> lambda(Fn&& function)
    {
        static_assert(!is_fun_ptr<std::decay_t<Fn>>::value, "use lambda");
        static_assert(!std::is_member_function_pointer<std::decay_t<Fn>>::value, "use lambda");
        using Binder = Binder<false, std::decay_t<Fn>>;
        using UnboundRunType = typename funcion_traits_unbound_runtype<Fn>::unbound_runtype;
        using Invoker = Invoker<false, Binder, UnboundRunType>;
        using Callback = Callback<UnboundRunType>;
        using invoker_fun_type = BinderBase::invoker_fun_type;
        return Callback(std::shared_ptr<BinderBase>(std::make_shared<Binder>(reinterpret_cast<invoker_fun_type>(&Invoker::invoke),
            std::forward<Fn>(function))));
    }
}


namespace Storm
{
    template <class R, class ...A>
    Delegate<R(A ...)> delegate(R(*function)(A ...))
    {
        return Storm::bind(function);
    }

    template <class C, class T, class R, class ...A>
    Delegate<R(A ...)> delegate(R(C::*function)(A ...), T&& object)
    {
        return Storm::bind(function, std::forward<T>(object));
    }

    template <class C, class T, class R, class ...A>
    Delegate<R(A ...)> delegate(R(C::*function)(A ...) const, T&& object)
    {
        return Storm::bind(function, std::forward<T>(object));
    }

    template <class F, typename = std::enable_if_t<!std::is_member_function_pointer<std::decay_t<F>>::value, bool>>
    Delegate<function_signature_t<std::decay_t<F>>> delegate(F&& function)
    {
        return Storm::bind(std::forward<F>(function));
    }
}


namespace Storm
{
    namespace priv
    {
        class dummy_lock
        {
        public:
            void lock() {}
            void unlock() {}
        };

    }

    typedef priv::dummy_lock st_policy;
    typedef std::mutex mt_policy;

    typedef st_policy def_thread_policy;

    template<class ThreadPolicy>
    class Trackable;

    template <class, class>
    class Event;

    class ConnectionConextBase;

    template<class T, class ThreadPolicy>
    class ConnectionContext;

    template<class ThreadPolicy>
    class EventBase
    {
        EventBase(const EventBase &) = delete;
        EventBase &operator=(const EventBase &) = delete;
    public:
        EventBase() = default;
        virtual ~EventBase() {}
        virtual void connectionDisconnect(ConnectionConextBase*) = 0;
        virtual void trackableExpired(Trackable<ThreadPolicy>* object) = 0;
    };

    class ConnectionConextBase : public std::enable_shared_from_this<ConnectionConextBase>
    {
    protected:
        std::atomic<bool> connected_ = true;
    public:
        ConnectionConextBase() : connected_(true) {}
        virtual ~ConnectionConextBase() {}

        virtual void disconnectSelf() = 0;
        virtual void trackableExpired() = 0;
        virtual void lock() = 0;
        virtual void unlock() = 0;

        bool isConnected() const { return connected_; }

        void disconnect()
        {
            //disconnect from event 
            //so this will require lock
            disconnectSelf();
            connected_ = false;
        }

        void disconnected()
        {
            std::lock_guard<ConnectionConextBase> lock(*this);
            connected_ = false;
        }

        void nolockDisconnected()
        {
            connected_ = false;
        }
    };


    template<class R, class ...A, class ThreadPolicy>
    class ConnectionContext<R(A...), ThreadPolicy> : public ConnectionConextBase
    {
        EventBase<ThreadPolicy>* event_;
        Delegate<R(A...)> delegate_;
        Trackable<ThreadPolicy>* contextObject_;
        ThreadPolicy lock_;
    public:
        ConnectionContext(EventBase<ThreadPolicy>* e, const Delegate<R(A...)>& d, Trackable<ThreadPolicy>* t)
            : event_(e), delegate_(d), contextObject_(t)
        {

        }

        ConnectionContext(EventBase<ThreadPolicy>* e, Delegate<R(A...)>&& d, Trackable<ThreadPolicy>* t)
            : event_(e), delegate_(std::move(d)), contextObject_(t)
        {

        }

        void disconnectSelf() override
        {
            std::lock_guard<ConnectionContext> lock(*this);
            if (isConnected())
            {
                nolockDisconnected();
                event_->connectionDisconnect(this);
                event_ = nullptr;
                contextObject_ = nullptr;
            }
        }

        void trackableExpired() override
        {
            std::lock_guard<ConnectionContext> lock(*this);
            if (isConnected())
            {
                nolockDisconnected();
                event_->trackableExpired(contextObject_);
                event_ = nullptr;
                contextObject_ = nullptr;
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
                delegate_(std::forward<A>(args)...);
            }
        }

        auto getDelegate() const
        {
            return delegate_;
        }

        Trackable<ThreadPolicy>* getTrackable()
        {
            std::lock_guard<ConnectionContext> lock(*this);
            return contextObject_;
        }

        Trackable<ThreadPolicy>* nolockGetTrackable() const
        {
            return contextObject_;
        }
    };

    class Connection
    {
    protected:
        std::weak_ptr<ConnectionConextBase> context_;
    public:
        Connection(const std::weak_ptr<ConnectionConextBase>& d)
            :context_(std::weak_ptr<ConnectionConextBase>(d))
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

    template<class ThreadPolicy = def_thread_policy>
    class Trackable
    {
        Trackable(const Trackable &) = delete; 
        Trackable &operator=(const Trackable &) = delete;
        ThreadPolicy trackableLock_;
        std::set<std::shared_ptr<ConnectionConextBase>> connections_;
    public:
        Trackable() = default;
        virtual ~Trackable()
        {
            removeAll();
        }

        void connectionAdded(const std::shared_ptr<ConnectionConextBase> & conn)
        {
            std::lock_guard<ThreadPolicy> lock(trackableLock_);
            connections_.insert(conn);
        }

        void connectionRemoved(const std::shared_ptr<ConnectionConextBase> & conn)
        {
            std::lock_guard<ThreadPolicy> lock(trackableLock_);
            auto it = connections_.find(conn);
            if (it != connections_.end())
            {
                connections_.erase(it);
            }
        }

        void removeAll()
        {
            std::lock_guard<ThreadPolicy> lock(trackableLock_);
            for (auto it = connections_.begin(); it != connections_.end(); ++it)
            {
                (*it)->trackableExpired();
            }
            connections_.clear();
        }

    };

    template <class R, class ...A, class ThreadPolicy>
    class Event<R(A ...), ThreadPolicy> : public EventBase<ThreadPolicy>
    {
        using Delegate = Delegate<R(A ...)>;
        using ConnectionContext = ConnectionContext<R(A ...), ThreadPolicy>;
        std::shared_ptr<std::vector<std::shared_ptr<ConnectionContext>>> connections_;
        ThreadPolicy lock_;

        void compactConnections()
        {
            //make sure operation on connections is safe
            if (!connections_.unique())
            {
                connections_.reset(new std::vector<std::shared_ptr<ConnectionContext>>(*connections_));
            }
        }

        Connection addConnection(const std::shared_ptr<ConnectionContext>& conn, Trackable<ThreadPolicy>* trackable = nullptr)
        {
            compactConnections();

            connections_->emplace_back(conn);
            if (trackable)
            {
                trackable->connectionAdded(conn);
            }
            return Connection(conn);
        }

        void connectionDisconnect(ConnectionConextBase* conn) override
        {
            std::lock_guard<ThreadPolicy> lock(lock_);
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

        void trackableExpired(Trackable<ThreadPolicy>* object) override
        {
            std::lock_guard<ThreadPolicy> lock(lock_);
            compactConnections();

            auto it = connections_->begin();
            for (; it != connections_->end();)
            {
                if ((*it)->nolockGetTrackable() == object)
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
        Event() : connections_(std::make_shared<std::vector<std::shared_ptr<ConnectionContext>>>())
        {

        }

        ~Event()
        {
            removeAll();
        }

        Connection add(Delegate&& delegate)
        {
            std::lock_guard<ThreadPolicy> lock(lock_);
            return addConnection(std::make_shared<ConnectionContext>(this, std::move(delegate), nullptr));
        }

        Connection add(const Delegate& delegate)
        {
            std::lock_guard<ThreadPolicy> lock(lock_);
            return addConnection(std::make_shared<ConnectionContext>(this, delegate, nullptr));
        }

        template<class C, class O, typename = std::enable_if_t<std::is_base_of<Trackable<ThreadPolicy>, std::remove_cv_t<std::remove_reference_t<O>>>::value, bool>>
        Connection add(R(C::*function)(A ...), O* object)
        {
            std::lock_guard<ThreadPolicy> lock(lock_);
            return addConnection(std::make_shared<ConnectionContext>(this, Storm::delegate(function, object), object), object);
        }

        template<class O, typename = std::enable_if_t<std::is_base_of<Trackable<ThreadPolicy>, std::remove_cv_t<std::remove_reference_t<O>>>::value, bool>>
        Connection add(const Delegate& delegate, O* object)
        {
            std::lock_guard<ThreadPolicy> lock(lock_);
            return addConnection(std::make_shared<ConnectionContext>(this, delegate, object), object);
        }

        template<class O, typename = std::enable_if_t<std::is_base_of<Trackable<ThreadPolicy>, std::remove_cv_t<std::remove_reference_t<O>>>::value, bool>>
        Connection add(Delegate&& delegate, O* object)
        {
            std::lock_guard<ThreadPolicy> lock(lock_);
            return addConnection(std::make_shared<ConnectionContext>(this, std::move(delegate), object), object);
        }

        void remove(Trackable<ThreadPolicy>* object)
        {
            std::lock_guard<ThreadPolicy> lock(lock_);
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

        void remove(const Delegate& delegate)
        {
            std::lock_guard<ThreadPolicy> lock(lock_);
            compactConnections();

            auto it = connections_->begin();
            for (; it != connections_->end();)
            {
                if ((*it)->getDelegate() == delegate)
                {
                    auto conn = *it;
                    auto object = conn->getTrackable();
                    conn->disconnected();
                    it = connections_->erase(it);
                    if (object)
                    {
                        object->connectionRemoved(conn);
                    }
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
            std::lock_guard<ThreadPolicy> lock(lock_);

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

        void operator +=(const Delegate& delegate)
        {
            add(delegate);
        }

        void operator +=(Delegate&& delegate)
        {
            add(std::move(delegate));
        }

        void operator -=(const Delegate& delegate)
        {
            remove(delegate);
        }

        void emit(arg_traits_t<A>...args)
        {
            std::shared_ptr<std::vector<std::shared_ptr<ConnectionContext>>> connections;
            {
                std::lock_guard<ThreadPolicy> lock(lock_);
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

    template<class S, class ThreadPolicy = def_thread_policy>
    class Event : public Event<S, ThreadPolicy>
    {
    };

}
