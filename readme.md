SimplifiedCppIDiom
=======
Collection of useful c++ tools / common idioms with clean code, easy to use api, mose of then are just one file with minimal dependcy.


## How to

Just include the corresponding file.

## Useful kits

##### [signal slot](https://github.com/hiitiger/simplifiedCppIdiom/blob/master/object/signal_slot_track.h)
A thread safe and trackable signal slot implementation with no external dependency, less than 400 lines of code.

```c++
signal<void(const std::string&, std::string&&)> signal1;
connection conn = signal1.connect([](const std::string v, std::string&& str) {
        std::cout << << v << str << std::endl;
    });
```

##### [com ptr](https://github.com/hiitiger/simplifiedCppIdiom/blob/master/object/comptr.h)
Windows Com ptr implementation offer clean and safe interface.

##### [object tree](https://github.com/hiitiger/simplifiedCppIdiom/blob/master/object/objecttree.h)
Object tree is a useful way to manage object in c++.

##### [Qt metecall](https://github.com/hiitiger/simplifiedCppIdiom/blob/master/adapter/qt/metacall.h)
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

##### [Qt genericsignalmap](https://github.com/hiitiger/simplifiedCppIdiom/blob/master/adapter/qt/genericsignalmap.h)
Generica Qt signal map which enables user to connect QObject's signal with name and QVariantList packed arguments.

```c++
GenericSignalMapper* mapper1 = new GenericSignalMapper(method, this);
connect(object, qFlagLocation(signature.toUtf8().constData()), mapper1, SLOT(mapSlot()));
connect(mapper1, SIGNAL(mapped(QObject*, QMetaMethod, QVariantList)), this, SLOT(onGenericSignal(QObject*, QMetaMethod, QVariantList)));

```


##### [qasync](https://github.com/hiitiger/simplifiedCppIdiom/blob/master/adapter/qt/qasync.h)
An async call adapter for Qt which enables user to post async lambda call to Qt's UI thread.

##### [workerpool](https://github.com/hiitiger/simplifiedCppIdiom/blob/master/thread/workerpool.h)
A c++11 thread pool.

##### [ppl async adapter](https://github.com/hiitiger/simplifiedCppIdiom/blob/master/adapter/ppl/appasync.h)
A wrapper for ppltask which enable user write async code like this:


```c++
using namespace concurrency_;

auto t = delayed(1000)
| ui([] {
    std::cout << "running on ui" << std::endl;
})
| pool([] {
    std::cout << "running on pool" << std::endl;
    return std::this_thread::get_id();
})
| delay<std::thread::id>(100)
| pool([](std::thread::id id) {
    std::cout << "running on pool" << std::endl;
    std::cout <<  "received  thread_id" << id  << std::endl;
});

```

##### [snowflake](https://github.com/hiitiger/simplifiedCppIdiom/blob/master/tool/snowflake.h)
Snowflake uuid generator in c++.

##### [throttle](https://github.com/hiitiger/simplifiedCppIdiom/blob/master/tool/throttle.h)
A simple throttle control.

##### [trace](https://github.com/hiitiger/simplifiedCppIdiom/tree/master/trace)
Object leak trace, perf timer and extras.
