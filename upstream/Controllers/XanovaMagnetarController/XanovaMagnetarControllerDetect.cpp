/*---------------------------------------------------------*\
| XanovaMagnetarControllerDetect.cpp                        |
|                                                           |
|   Detector for Xanova Magnetar RGB Mechanical Keyboard    |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#include "Detector.h"
#include "XanovaMagnetarController.h"
#include "RGBController_XanovaMagnetar.h"

#define XANOVA_VID              0x1024
#define XANOVA_MAGNETAR_PID     0x2030

void DetectXanovaMagnetarControllers(hid_device_info* info, const std::string& name)
{
    hid_device* dev = hid_open_path(info->path);

    if(dev)
    {
        XanovaMagnetarController*     controller     = new XanovaMagnetarController(dev, *info, name);
        RGBController_XanovaMagnetar* rgb_controller = new RGBController_XanovaMagnetar(controller);

        ResourceManager::get()->RegisterRGBController(rgb_controller);
    }
}

REGISTER_HID_DETECTOR_IPU("Xanova Magnetar RGB", DetectXanovaMagnetarControllers, XANOVA_VID, XANOVA_MAGNETAR_PID, 2, 0x0001, 0x0002);
