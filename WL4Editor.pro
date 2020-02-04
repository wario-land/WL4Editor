#-------------------------------------------------
#
# Project created by QtCreator 2018-05-20T20:34:45
#
#-------------------------------------------------

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = WL4Editor
TEMPLATE = app

include(./ThirdParty/phantomstyle/src/phantom/phantom.pri)

RC_ICONS = images/icon.ico

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11 strict_c++

QMAKE_CXXFLAGS = -fpermissive

SOURCES += \
    Dialog/TilesetEditor_Tile8x8EditorGraphicView.cpp \
    main.cpp \
    WL4EditorWindow.cpp \
    LevelComponents/Level.cpp \
    LevelComponents/Room.cpp \
    LevelComponents/Layer.cpp \
    LevelComponents/Door.cpp \
    LevelComponents/Tile.cpp \
    LevelComponents/Tileset.cpp \
    ROMUtils.cpp \
    Operation.cpp \
    Dialog/ChooseLevelDialog.cpp \
    DockWidget/Tile16DockWidget.cpp \
    DockWidget/TileGraphicsView.cpp \
    EditorWindow/MainGraphicsView.cpp \
    LevelComponents/EntitySet.cpp \
    Dialog/LevelConfigDialog.cpp \
    DockWidget/EditModeDockWidget.cpp \
    Dialog/RoomConfigDialog.cpp \
    Dialog/RoomPreviewGraphicsView.cpp \
    Dialog/DoorConfigDialog.cpp \
    LevelComponents/Entity.cpp \
    DockWidget/EntitySetDockWidget.cpp \
    Compress.cpp \
    DockWidget/CameraControlDockWidget.cpp \
    Dialog/PatchManagerDialog.cpp \
    Dialog/PatchManagerTableView.cpp \
    PatchUtils.cpp \
    Dialog/PatchEditDialog.cpp \
    Dialog/TilesetEditDialog.cpp \
    SettingsUtils.cpp \
    Dialog/TilesetEditor_Tile16MapGraphicView.cpp \
    Dialog/TilesetEditor_Tile8x8MapGraphicView.cpp \
    Dialog/TilesetEditor_PaletteGraphicView.cpp

HEADERS += \
    Dialog/TilesetEditor_Tile8x8EditorGraphicView.h \
    WL4EditorWindow.h \
    LevelComponents/Level.h \
    LevelComponents/Room.h \
    LevelComponents/Layer.h \
    LevelComponents/Door.h \
    ROMUtils.h \
    WL4Constants.h \
    LevelComponents/Tile.h \
    LevelComponents/Tileset.h \
    Dialog/ChooseLevelDialog.h \
    DockWidget/Tile16DockWidget.h \
    DockWidget/TileGraphicsView.h \
    EditorWindow/MainGraphicsView.h \
    LevelComponents/EntitySet.h \
    Dialog/LevelConfigDialog.h \
    DockWidget/EditModeDockWidget.h \
    Operation.h \
    Dialog/RoomConfigDialog.h \
    Dialog/RoomPreviewGraphicsView.h \
    Dialog/DoorConfigDialog.h \
    LevelComponents/Entity.h \
    DockWidget/EntitySetDockWidget.h \
    Compress.h \
    DockWidget/CameraControlDockWidget.h \
    WL4Application.h \
    Dialog/PatchManagerDialog.h \
    Dialog/PatchManagerTableView.h \
    PatchUtils.h \
    Dialog/PatchEditDialog.h \
    Dialog/TilesetEditDialog.h \
    SettingsUtils.h \
    Dialog/TilesetEditor_Tile16MapGraphicView.h \
    Dialog/TilesetEditor_Tile8x8MapGraphicView.h \
    Dialog/TilesetEditor_PaletteGraphicView.h \
    ThirdParty/phantomstyle/src/phantom/phantomstyle.h \
    Themes.h

FORMS += \
    WL4EditorWindow.ui \
    Dialog/ChooseLevelDialog.ui \
    DockWidget/Tile16DockWidget.ui \
    Dialog/LevelConfigDialog.ui \
    DockWidget/EditModeDockWidget.ui \
    Dialog/RoomConfigDialog.ui \
    Dialog/DoorConfigDialog.ui \
    DockWidget/EntitySetDockWidget.ui \
    DockWidget/CameraControlDockWidget.ui \
    Dialog/PatchManagerDialog.ui \
    Dialog/PatchEditDialog.ui \
    Dialog/TilesetEditDialog.ui
