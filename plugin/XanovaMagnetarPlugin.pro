TARGET          = XanovaMagnetarPlugin
TEMPLATE        = lib
CONFIG         += plugin c++17

QT             += core gui widgets

INCLUDEPATH    += . \
                  OpenRGB \
                  OpenRGB/RGBController

HEADERS        += \
    OpenRGB/OpenRGBPluginInterface.h \
    OpenRGB/ResourceManagerInterface.h \
    OpenRGB/RGBController/RGBController.h \
    OpenRGB/RGBController/RGBControllerKeyNames.h \
    XanovaMagnetarController.h \
    RGBController_XanovaMagnetar.h \
    XanovaMagnetarPlugin.h

SOURCES        += \
    OpenRGB/RGBController/RGBController.cpp \
    OpenRGB/RGBController/RGBControllerKeyNames.cpp \
    XanovaMagnetarController.cpp \
    RGBController_XanovaMagnetar.cpp \
    XanovaMagnetarPlugin.cpp

win32 {
    LIBS += -lhid -lsetupapi -luser32
}
