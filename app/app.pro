#-------------------------------------------------
#
# Project created by wangshuange
#
#-------------------------------------------------

QT       += core gui xml svg serialport
QT += charts multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

include(../qtpropertybrowser/src/qtpropertybrowser.pri)
TARGET = robot_control_ex
TEMPLATE = app
SOURCES += main.cpp\
        mainwindow.cpp \
    drawobj.cpp \
    drawscene.cpp \
    drawtool.cpp \
    sizehandle.cpp \
    objectcontroller.cpp \
    customproperty.cpp \
    rulebar.cpp \
    drawview.cpp \
    commands.cpp \
    document.cpp \
    mycombobox.cpp \
    myplay.cpp \
    mychart.cpp \
    customslider.cpp \
    QxtSpanSlider.cpp \
    servoinfo.cpp \
    xmltool.cpp \
    actiongroup.cpp \
    motoraction.cpp

HEADERS  += mainwindow.h \
    drawobj.h \
    drawscene.h \
    drawtool.h \
    sizehandle.h \
    objectcontroller.h \
    customproperty.h \
    rulebar.h \
    drawview.h \
    commands.h \
    document.h \
    mycombobox.h \
    myplay.h \
    mychart.h \
    customslider.h \
    QxtSpanSlider.h \
    QxtSpanSlider_p.h \
    servoinfo.h \
    xmltool.h \
    actiongroup.h \
    motoraction.h

RESOURCES += \
    app.qrc
INCLUDEPATH +=/usr/local/ffmpeg/include
LIBS +=-L /usr/local/ffmpeg/lib -lavcodec -lavformat -lswscale -lavutil -lswresample -lSDL2
