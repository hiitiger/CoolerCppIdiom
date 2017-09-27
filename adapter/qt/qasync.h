#pragma once
#include <QtCore/QtCore>

namespace Qx
{
    void async(const std::function<void()>& cb);
    void asyncDelayed(const std::function<void()>& cb, int milliSecs);
}