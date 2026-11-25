/*---------------------------------------------------------*\
| RGBController_XanovaMagnetar.h                            |
|                                                           |
|   RGBController for Xanova Magnetar RGB Keyboard          |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#pragma once

#include "RGBController.h"
#include "XanovaMagnetarController.h"

class RGBController_XanovaMagnetar : public RGBController
{
public:
    explicit RGBController_XanovaMagnetar(XanovaMagnetarController* controller_ptr);
    ~RGBController_XanovaMagnetar() override;

    void SetupZones() override;
    void ResizeZone(int zone, int new_size) override;

    void DeviceUpdateLEDs() override;
    void UpdateZoneLEDs(int zone) override;
    void UpdateSingleLED(int led) override;

    void DeviceUpdateMode() override;

private:
    XanovaMagnetarController* controller;
};
