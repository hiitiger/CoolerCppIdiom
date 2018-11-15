TEMPLATE = app
#CONFIG -= qt
QT += core
CONFIG += console

QMAKE_CXXFLAGS += /await
QMAKE_CXXFLAGS += /std:c++latest

PRECOMPILED_HEADER = stable.h
SOURCES += $$files(main.cpp)

INCLUDEPATH += ./

HEADERS += $$files(object/*.h)
SOURCES += $$files(object/*.cpp)

HEADERS += $$files(container/*.h)
SOURCES += $$files(container/*.cpp)

HEADERS += $$files(tool/*.h)
SOURCES += $$files(tool/*.cpp)

HEADERS += $$files(time/*.h)
SOURCES += $$files(time/*.cpp)

HEADERS += $$files(thread/*.h)
SOURCES += $$files(thread/*.cpp)

HEADERS += $$files(adapter/*.h)
SOURCES += $$files(adapter/*.cpp)

#HEADERS += $$files(adapter/qt/*.h)
#SOURCES += $$files(adapter/qt/*.cpp)

HEADERS += $$files(example/*.h)
SOURCES += $$files(example/*.cpp)


HEADERS += adapter/qt/qasync.h
SOURCES += adapter/qt/qasync.cpp

HEADERS += adapter/qt/metacall.h
SOURCES += adapter/qt/metacall.cpp

SOURCES += adapter/qt/genericsignalmap.cpp


equals(QT_MAJOR_VERSION, 4) {
    message( "Building for Qt4")
    SOURCES += adapter/qt/moc_genericsignalmap_4.cpp
}

equals(QT_MAJOR_VERSION, 5) {
    message( "Building for Qt5")
    SOURCES += adapter/qt/moc_genericsignalmap_5.cpp
}


HEADERS += adapter/ppl/appasync.h
SOURCES += adapter/ppl/appasync.cpp

HEADERS += json/nlohmann/json.hpp
HEADERS += json/json_auto.h
