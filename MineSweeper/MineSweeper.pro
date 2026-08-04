QT += widgets network

CONFIG += c++17
CONFIG -= debug_and_release
CONFIG += release

TEMPLATE = app
TARGET = MineSweeper
DESTDIR = bin

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    src/main.cpp \
    src/MineField.cpp \
    src/BoardWidget.cpp \
    src/MainWindow.cpp \
    src/CustomGameDialog.cpp

HEADERS += \
    src/MineField.h \
    src/BoardWidget.h \
    src/MainWindow.h \
    src/CustomGameDialog.h

# MSVC: treat source files as UTF-8
msvc {
    QMAKE_CXXFLAGS += /utf-8
}
