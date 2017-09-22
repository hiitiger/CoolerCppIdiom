#pragma once
class CowData : public std::enable_shared_from_this<CowData>
{
    CowData& operator=(const CowData&) = delete;
public:
    CowData()
    {
        ;
    }
    CowData(const CowData&)
    {
        ;
    }
};


template<class T >
class CowPtr
{
    std::shared_ptr<T> dptr_;
public:
    explicit CowPtr(T* data = nullptr)
        :dptr_(data)
    {

    }

    CowPtr(const CowPtr& other)
        :dptr_(other.dptr_)
    {

    }

    CowPtr(CowPtr&& other)
        : dptr_(std::move(other.dptr_))
    {

    }

    CowPtr & operator=(const CowPtr &other)
    {
        CowPtr(other).swap(*this);
        return *this;
    }

    CowPtr& operator=(CowPtr&& other)
    {
        CowPtr(std::move(other)).swap(*this);
        return *this;
    }

    void detach()
    {
        if (dptr_)
        {
            if (!dptr_.unique())
            {
                dptr_ = std::shared_ptr<T>(new T(*dptr_));
            }
        }
    }

    T &operator*() { detach(); return *dptr_; }
    const T &operator*() const { return *dptr_; }

    T *operator->() { detach(); return dptr_.operator->(); }
    const T *operator->() const { return dptr_.operator->(); }

    operator T *() { detach(); return dptr_.get(); }
    operator const T *() const { return dptr_.get(); }

    T *data() { detach(); return dptr_.get(); }
    const T *data() const { return dptr_.get(); }
    const T *constData() const { return dptr_.get(); }

    bool operator!() const
    {
        return !dptr_;
    }

private:
    void swap(CowPtr& other)
    {
        std::swap(dptr_, other.dptr_);
    }

};
