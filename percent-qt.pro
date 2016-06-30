QT       += core gui widgets

LIBS += -lopencv_core -lopencv_highgui -lopencv_imgproc

TARGET = percent-qt
CONFIG -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11
QMAKE_LFLAGS += -std=c++11

SOURCES += main.cpp \
    MainWindow.cpp

HEADERS += \
    MainWindow.h
