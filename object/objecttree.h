#pragma once

namespace object_tree
{

class Object
{
    Object(const Object&) = delete;
    Object& operator==(const Object&) = delete;
public:
    explicit Object(Object* parent = nullptr);
    ~Object();

    void setParent(Object* parent);
    Object* parent() const;

    const std::vector<Object*>& children() const;

private:
    void deleteChildren();

private:
    Object* parent_ = nullptr;
    std::vector<Object*> children_;
};

inline Object::Object(Object* parent /*= nullptr*/)
    : parent_(parent)
{
    if (parent_)
    {
        parent_->children_.push_back(this);
    }
}

inline Object::~Object()
{
    deleteChildren();
    setParent(nullptr);
}

inline void Object::setParent(Object* parent)
{
    if (parent != parent_ && parent != this)
    {
        if (parent_)
        {
            auto it = std::find(parent_->children_.begin(), parent_->children_.end(), this);
            if (it != parent_->children_.end())
            {
                parent_->children_.erase(it);
            }
            parent_ = nullptr;
        }

        if (parent)
        {
            parent_ = parent;
            parent_->children_.push_back(this);
        }
    }
}

inline Object* Object::parent() const
{
    return parent_;
}

inline const std::vector<Object*>& Object::children() const
{
    return children_;
}

inline void Object::deleteChildren()
{
    for (auto& child : children_)
    {
        child->parent_ = nullptr;
        delete child;
        child = nullptr;
    }
    children_.clear();
}

}
