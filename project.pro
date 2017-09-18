TEMPLATE = app
CONFIG -= qt
CONFIG += console

PRECOMPILED_HEADER = stable.h
SOURCES += $$files(main.cpp)

HEADERS += $$files(object/*.h)
SOURCES += $$files(object/*.cpp)

HEADERS += $$files(container/*.h)
SOURCES += $$files(container/*.cpp)

HEADERS += $$files(tool/*.h)
SOURCES += $$files(tool/*.cpp)

HEADERS += $$files(example/*.h)
SOURCES += $$files(example/*.cpp)