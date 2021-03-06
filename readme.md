CoolerCppIdiom
=======

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/da1719248d20475e91623887977f9f54)](https://www.codacy.com/app/hiitiger/CoolerCppIdiom?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=hiitiger/CoolerCppIdiom&amp;utm_campaign=Badge_Grade)

Collection of useful c++ tools / common idioms you might not found elsewhere, provide with clean code and easy to use api, mose of then are just one file with minimal dependency.


## How to

Just include the corresponding file.

## Useful kits

##### [cooler cpp async](https://github.com/hiitiger/CoolerCppIdiom/blob/master/adapter/ppl/appasync.h)
A cooler version of promise pattern, we can create our own dispatcher to dispatch task to threads(ui,pool,io,http,etc) just like using std::async and future<T>::then.

This specific implementation uses ppltask, but we can implement it on any primise-like library for c++.

```c++
using namespace concurrency_;

auto t = delayed(1000)
| ui([] {
    std::cout << "running on ui" << std::endl;
})
| io([] {
    std::cout << "running on io" << std::endl;
    return std::this_thread::get_id();
})
| delay<std::thread::id>(100)
| pool([](std::thread::id id) {
    std::cout << "running on pool" << std::endl;
    std::cout << "received thread_id" << id  << std::endl;
});

```

##### [json auto serialize](https://github.com/hiitiger/CoolerCppIdiom/blob/master/json/json_auto.h)

Make json serialization easier for your life.

With nlohmann's [JSON for Modern C++](https://github.com/nlohmann/json) plus a helper file, we can do this now

```c++
struct Person{
    std::string name;
    std::optional<std::uint32_t> age;
    std::optional<std::vector<Person>> friends;
};

JSON_AUTO(Person, name, age, friends)

void func()
{
    Person p;
    p.name = "John";
    json obj = p;
    Person op = obj;
}
```

##### [Event Delegate](https://github.com/hiitiger/CoolerCppIdiom/blob/master/object/event.h)
A simple yet powerful event delegate implementation, support trackable listener, provide an alternate bind to std::bind which support bind to smart pointer.

```c++
Event<void(const std::string&, std::string&&)> signal1;

auto func1 = Storm::lambda([&, nocopy]() {
    signal1("123", "123");
});

auto conn1 = signal1.add([&slot1](const std::string& v, std::string&& str){
    slot1 = true;
    std::cout << "\nslot 1: " << v << ", " << std::string(std::move(str)) << std::endl;
});

Connection conn2 = signal1.add([&slot2, &conn2](const std::string& v, std::string&& str) {
    slot2 = true;
    std::cout << "\nslot 2: " << v << ", " << str << std::endl;
    conn2.disconnect();
});


func1();
```

##### [qasync](https://github.com/hiitiger/CoolerCppIdiom/blob/master/adapter/qt/qasync.h)
An async call adapter for Qt which enables user to post async lambda to Qt's UI thread.

##### [workerpool](https://github.com/hiitiger/CoolerCppIdiom/blob/master/thread/workerpool.h)
A easy to use c++11 thread pool.

##### [snowflake](https://github.com/hiitiger/CoolerCppIdiom/blob/master/tool/snowflake.h)
Snowflake uuid generator in c++.

##### [throttle](https://github.com/hiitiger/CoolerCppIdiom/blob/master/tool/throttle.h)
A simple throttle control.

##### [trace](https://github.com/hiitiger/CoolerCppIdiom/tree/master/trace)
Object leak trace, perf timer and extras.

##### [time](https://github.com/hiitiger/CoolerCppIdiom/tree/master/time)
Timetick, Datetime, FpsTimer.

##### [com ptr](https://github.com/hiitiger/CoolerCppIdiom/blob/master/object/comptr.h)
Windows Com ptr implementation with clean and safe interface.

##### [object tree](https://github.com/hiitiger/CoolerCppIdiom/blob/master/object/objecttree.h)
Object tree is a useful way to manage object in c++.

##### [Qt metecall](https://github.com/hiitiger/CoolerCppIdiom/blob/master/adapter/qt/metacall.h)
Generica Qt metacall which enables user to call QObject's method with name and QVariantList packed arguments.

```c++
...
const QString& object;
const QString& method; 
const QVariantList& args;
//get meta method from method name
QMetaMethod metaMethod;
Qx::metaCall(object, metaMethod, args);

```

##### [Qt genericsignalmap](https://github.com/hiitiger/CoolerCppIdiom/blob/master/adapter/qt/genericsignalmap.h)
Generica Qt signal map which enables user to connect QObject's signal with name and QVariantList packed arguments.

```c++
GenericSignalMapper* mapper1 = new GenericSignalMapper(method, this);
connect(object, qFlagLocation(signature.toUtf8().constData()), mapper1, SLOT(mapSlot()));
connect(mapper1, SIGNAL(mapped(QObject*, QMetaMethod, QVariantList)), this, SLOT(onGenericSignal(QObject*, QMetaMethod, QVariantList)));

```
