#-------------------------------------------------
#
# Project created by QtCreator 2021-02-05T16:00:19
#
#-------------------------------------------------

QT       += core gui
QT       += opengl
QT += 3dcore 3drender 3dinput 3dextras
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

QT       += serialport


TARGET = Human3dApp
TEMPLATE = app

DESTDIR = $$PWD/bin

CONFIG += c++11

SOURCES += main.cpp\
        mainwindow.cpp \
    stlfileloader.cpp \
    rrglwidget.cpp \
    humangl.cpp \
    sensorloader.cpp \
    qcustomplot.cpp \
    HotPlot.cpp \
    dataupdater.cpp \
    dataupdater2.cpp

HEADERS  += mainwindow.h \
    stlfileloader.h \
    rrglwidget.h \
    humangl.h \
    sensorloader.h \
    qcustomplot.h \
    HotPlot.h \
    dataupdater.h \
    globals.h \
    dataupdater2.h

FORMS    += mainwindow.ui

DISTFILES +=

#LIBS += -lglut -lGLU
LIBS += -lfreeglut -lopengl32 -lglu32

win32:CONFIG(release, debug|release): LIBS += -L'F:/Program Files/msys64/mingw64/lib/'
else:win32:CONFIG(debug, debug|release): LIBS += -L'F:/Program Files/msys64/mingw64/lib/'
INCLUDEPATH += 'F:/Program Files/msys64/mingw64/include'
DEPENDPATH += 'F:/Program Files/msys64/mingw64/include'

RESOURCES += \
    images.qrc
# QXlsx code for Application Qt project
QXLSX_PARENTPATH=./QXlsx/         # current QXlsx path is . (. means curret directory)
QXLSX_HEADERPATH=./QXlsx/header/  # current QXlsx header path is ./header/
QXLSX_SOURCEPATH=./QXlsx/source/  # current QXlsx source path is ./source/
include(./QXlsx/QXlsx.pri)

RC_ICONS = bitbug_favicon.ico




