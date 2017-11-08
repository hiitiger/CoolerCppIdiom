#pragma once
#include <QtCore/QtCore>

class QxApplication : public QCoreApplication
{
    Q_OBJECT;

public:
    QxApplication(int argc, char* argv[]);
    ~QxApplication();

    static QxApplication* instance();

    std::thread::id threadId();

protected:
    virtual bool event(QEvent*);

private slots:
    void runQueue();

private:
    friend class QxApplicationPrivate;
    QxApplicationPrivate * d_ptr;
};



namespace Qx
{
    void async(const std::function<void()>& cb);
    void asyncDelayed(const std::function<void()>& cb, int milliSecs);
    std::thread::id threadId();

}