/*---------------------------------------------------------*\
| XanovaMagnetarController.cpp                              |
|                                                           |
|   Driver for Xanova Magnetar RGB Mechanical Keyboard      |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#include "XanovaMagnetarController.h"
#include "StringUtils.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

using namespace std::chrono_literals;

XanovaMagnetarController::XanovaMagnetarController(hid_device* dev_handle, const hid_device_info& info, std::string dev_name)
    : dev(dev_handle),
      location(info.path ? info.path : ""),
      name(std::move(dev_name)),
      custom_mode_initialized(false)
{
    if(info.serial_number)
    {
        serial_number = StringUtils::wstring_to_string(info.serial_number);
    }
}

XanovaMagnetarController::~XanovaMagnetarController()
{
    std::lock_guard<std::mutex> lock(io_mutex);
    if(dev)
    {
        hid_close(dev);
        dev = nullptr;
    }
}

std::string XanovaMagnetarController::GetDeviceLocation()
{
    return "HID: " + location;
}

std::string XanovaMagnetarController::GetNameString()
{
    return name;
}

std::string XanovaMagnetarController::GetSerialString()
{
    return serial_number;
}

bool XanovaMagnetarController::SendFeatureRaw(unsigned char* report_65)
{
    if(!dev)
    {
        return false;
    }
    return hid_send_feature_report(dev, report_65, XANOVA_MAGNETAR_REPORT_LEN) == XANOVA_MAGNETAR_REPORT_LEN;
}

bool XanovaMagnetarController::SendChecksummedCommand(uint8_t b4, uint8_t b5, uint8_t b6, unsigned char* response_65)
{
    if(!dev)
    {
        return false;
    }

    unsigned char buf[XANOVA_MAGNETAR_REPORT_LEN]{};
    buf[4] = b4;
    buf[5] = b5;
    buf[6] = b6;

    uint8_t chk = 0;
    for(size_t i = 3; i < 63; ++i)
    {
        chk ^= buf[i];
    }
    buf[63] = chk;

    bool sent = false;
    for(int attempt = 0; attempt < 20; ++attempt)
    {
        if(SendFeatureRaw(buf))
        {
            sent = true;
            break;
        }
        std::this_thread::sleep_for(1ms);
    }
    if(!sent)
    {
        return false;
    }

    std::this_thread::sleep_for(5ms);
    for(int poll = 0; poll < 200; ++poll)
    {
        std::memset(buf, 0, XANOVA_MAGNETAR_REPORT_LEN);
        buf[0] = 0x00;

        int res = hid_get_feature_report(dev, buf, XANOVA_MAGNETAR_REPORT_LEN);
        if(res >= 10)
        {
            if(buf[1] == 0x02 && buf[4] == b4 && buf[5] == b5 && buf[6] == b6)
            {
                if(response_65)
                {
                    std::memcpy(response_65, buf, XANOVA_MAGNETAR_REPORT_LEN);
                }
                return true;
            }
            if(buf[1] == 0x03 && buf[4] == b4 && buf[5] == b5 && buf[6] == b6)
            {
                return false;
            }
        }
        std::this_thread::sleep_for(10ms);
    }

    return false;
}

std::string XanovaMagnetarController::GetFirmwareVersion()
{
    std::lock_guard<std::mutex> lock(io_mutex);
    unsigned char resp[XANOVA_MAGNETAR_REPORT_LEN]{};
    if(!SendChecksummedCommand(0x03, 0x00, 0x81, resp))
    {
        return "";
    }

    char ver[32]{};
    std::snprintf(ver, sizeof(ver), "%u.%u.%u", resp[7], resp[8], resp[9]);
    return std::string(ver);
}

uint8_t XanovaMagnetarController::GetDeviceLayout()
{
    std::lock_guard<std::mutex> lock(io_mutex);
    unsigned char resp[XANOVA_MAGNETAR_REPORT_LEN]{};
    if(!SendChecksummedCommand(0x01, 0x00, 0x85, resp))
    {
        return 0;
    }
    return resp[7];
}

bool XanovaMagnetarController::EnterCustomModeLocked()
{
    unsigned char hdr[XANOVA_MAGNETAR_REPORT_LEN]{};
    hdr[4] = 0x03;
    hdr[5] = 0x01;
    hdr[6] = 0x00;
    hdr[7] = 0xFE;

    if(!SendFeatureRaw(hdr))
    {
        return false;
    }

    if(!custom_mode_initialized)
    {
        std::this_thread::sleep_for(50ms);
        custom_mode_initialized = true;
    }
    return true;
}

void XanovaMagnetarController::SetLEDMatrix(const uint8_t matrix_rgb[XANOVA_MAGNETAR_MATRIX_ROWS][XANOVA_MAGNETAR_MATRIX_COLS][3])
{
    std::lock_guard<std::mutex> lock(io_mutex);

    if(!dev || !EnterCustomModeLocked())
    {
        return;
    }

    for(size_t ui_row = 0; ui_row < XANOVA_MAGNETAR_MATRIX_ROWS; ++ui_row)
    {
        uint8_t hw_row = static_cast<uint8_t>((XANOVA_MAGNETAR_MATRIX_ROWS - 1) - ui_row);

        unsigned char p1[XANOVA_MAGNETAR_REPORT_LEN]{};
        p1[4]  = 0x3A;
        p1[5]  = 0x01;
        p1[6]  = 0x03;
        p1[7]  = 0x06;
        p1[8]  = 0x16;
        p1[9]  = hw_row;
        p1[10] = 0x00;

        for(size_t c = 0; c < 18; ++c)
        {
            p1[11 + c * 3 + 0] = matrix_rgb[ui_row][c][0];
            p1[11 + c * 3 + 1] = matrix_rgb[ui_row][c][1];
            p1[11 + c * 3 + 2] = matrix_rgb[ui_row][c][2];
        }

        if(!SendFeatureRaw(p1))
        {
            return;
        }

        unsigned char p2[XANOVA_MAGNETAR_REPORT_LEN]{};
        p2[4]  = 0x10;
        p2[5]  = 0x01;
        p2[6]  = 0x03;
        p2[7]  = 0x06;
        p2[8]  = 0x16;
        p2[9]  = hw_row;
        p2[10] = 0x12;

        for(size_t c = 0; c < 4; ++c)
        {
            p2[11 + c * 3 + 0] = matrix_rgb[ui_row][18 + c][0];
            p2[11 + c * 3 + 1] = matrix_rgb[ui_row][18 + c][1];
            p2[11 + c * 3 + 2] = matrix_rgb[ui_row][18 + c][2];
        }

        if(!SendFeatureRaw(p2))
        {
            return;
        }
    }
}
