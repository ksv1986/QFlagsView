# QFlagsView

CONFIG   += c++11
QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QMAKE_CXXFLAGS += -Wall -Wextra
macx {
    QMAKE_CXXFLAGS += -stdlib=libc++
    QMAKE_CXXFLAGS += -mmacosx-version-min=10.7
}

TARGET = QFlagsView
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    core/flagsmodel.cpp \
    core/models.c

HEADERS  += mainwindow.h \
    core/flagsmodel.h \
    core/models.h

FORMS    += mainwindow.ui
