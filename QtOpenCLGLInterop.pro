#-------------------------------------------------
#
# Project created by QtCreator 2015-06-14T14:08:58
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QMAKE_CXXFLAGS += -std=c++11
TARGET = QtOpenCLGLInterop
TEMPLATE = app

LIBS+= -lOpenCL
LIBS+= -L"D:\Intel INDE\code_builder_5.1.0.25\lib\x86"
INCLUDEPATH+="D:\Intel INDE\code_builder_5.1.0.25\include"

SOURCES += main.cpp\
        mainwindow.cpp \
    myglwidget.cpp

HEADERS  += mainwindow.h \
    myglwidget.h

FORMS    += mainwindow.ui

DISTFILES += \
    metaballs.cl
