/*---------------------------------------------------------*\
| XanovaMagnetarController.h                                |
|                                                           |
|   Driver for Xanova Magnetar RGB Mechanical Keyboard      |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <hidapi.h>

#define XANOVA_MAGNETAR_REPORT_LEN  65
#define XANOVA_MAGNETAR_MATRIX_ROWS 6
#define XANOVA_MAGNETAR_MATRIX_COLS 22

class XanovaMagnetarController
{
public:
    XanovaMagnetarController(hid_device* dev_handle, const hid_device_info& info, std::string dev_name);
    ~XanovaMagnetarController();

    std::string GetDeviceLocation();
    std::string GetNameString();
    std::string GetSerialString();
    std::string GetFirmwareVersion();
    uint8_t     GetDeviceLayout();

    /**
     * Send a 6x22 RGB matrix in UI order (row 0 = top ESC row, row 5 = bottom CTRL row).
     * Internally flips rows to match hardware order (hw_row = 5 - ui_row).
     */
    void        SetLEDMatrix(const uint8_t matrix_rgb[XANOVA_MAGNETAR_MATRIX_ROWS][XANOVA_MAGNETAR_MATRIX_COLS][3]);

private:
    bool        SendFeatureRaw(unsigned char* report_65);
    bool        SendChecksummedCommand(uint8_t b4, uint8_t b5, uint8_t b6, unsigned char* response_65);
    bool        EnterCustomModeLocked();

    hid_device* dev;
    std::string location;
    std::string name;
    std::string serial_number;
    bool        custom_mode_initialized;
    std::mutex  io_mutex;
};
