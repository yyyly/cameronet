#-------------------------------------------------
#
# Project created by QtCreator 2019-03-30T10:08:18
#
#-------------------------------------------------

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT       += serialport


TARGET = CameroNet
TEMPLATE = lib

DEFINES += CAMERONET_LIBRARY


# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += D:\git\Screen

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        cameronet.cpp \
    logevent.cpp \
    enums.cpp \
    dhwork.cpp

HEADERS += \
        cameronet.h \
        cameronet_global.h \ 
    logevent.h \
    enums.h \
    cameradeviceimf.h \
    globaloption.h \
    dhconfigsdk.h \
    dhnetsdk.h \
    dhwork.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

include(log4qt/log4qt.pri)

LIBS += -L../CameroNet/lib -lHCNetSDK -lPlayCtrl -lHCCore
LIBS += -L../CameroNet/lib -llibScreen
LIBS += -L../CameroNet/lib -ldhnetsdk -ldhconfigsdk

FORMS +=
