#-------------------------------------------------
#
# Project created by QtCreator 2018-05-20T20:34:45
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = WL4Editor
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        WL4EditorWindow.cpp \
    LevelComponents/Level.cpp \
    LevelComponents/Room.cpp \
    LevelComponents/Layer.cpp \
    ROMUtils.cpp \
    LevelComponents/Door.cpp

HEADERS += \
        WL4EditorWindow.h \
    LevelComponents/Level.h \
    LevelComponents/Room.h \
    LevelComponents/Layer.h \
    LevelComponents/Door.h \
    ROMUtils.h \
    WL4Constants.h

FORMS += \
        WL4EditorWindow.ui
