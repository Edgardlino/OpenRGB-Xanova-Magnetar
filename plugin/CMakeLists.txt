cmake_minimum_required(VERSION 3.16)
project(XanovaMagnetarPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

add_library(XanovaMagnetarPlugin SHARED
    OpenRGB/RGBController/RGBController.cpp
    OpenRGB/RGBController/RGBControllerKeyNames.cpp
    XanovaMagnetarController.cpp
    RGBController_XanovaMagnetar.cpp
    XanovaMagnetarPlugin.cpp
    XanovaMagnetarPlugin.h
    RGBController_XanovaMagnetar.h
    XanovaMagnetarController.h
)

target_include_directories(XanovaMagnetarPlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/OpenRGB
    ${CMAKE_CURRENT_SOURCE_DIR}/OpenRGB/RGBController
)

target_compile_definitions(XanovaMagnetarPlugin PRIVATE QT_PLUGIN QT_SHARED)

target_link_libraries(XanovaMagnetarPlugin PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
)

if(WIN32)
    target_link_libraries(XanovaMagnetarPlugin PRIVATE hid setupapi user32)
endif()
