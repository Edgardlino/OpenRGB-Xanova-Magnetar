#include "XanovaMagnetarController.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <vector>

#include <setupapi.h>
extern "C"
{
#include <hidsdi.h>
}

namespace
{
    std::string ToLowerAscii(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return s;
    }

    std::string WideToUtf8(const wchar_t* wstr)
    {
        if (!wstr || !*wstr)
        {
            return "";
        }
        int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
        if (len <= 1)
        {
            return "";
        }
        std::string out(static_cast<size_t>(len - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr, -1, out.data(), len, nullptr, nullptr);
        return out;
    }
}

XanovaMagnetarController::XanovaMagnetarController()
    : handle_(INVALID_HANDLE_VALUE),
      custom_mode_initialized_(false)
{
}

XanovaMagnetarController::~XanovaMagnetarController()
{
    Close();
}

bool XanovaMagnetarController::Open()
{
    std::lock_guard<std::mutex> lock(io_mutex_);

    if (handle_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }
    custom_mode_initialized_ = false;
    location_.clear();

    GUID hid_guid{};
    HidD_GetHidGuid(&hid_guid);

    HDEVINFO dev_info = SetupDiGetClassDevsW(&hid_guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (dev_info == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    SP_DEVICE_INTERFACE_DATA if_data{};
    if_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (DWORD idx = 0; SetupDiEnumDeviceInterfaces(dev_info, nullptr, &hid_guid, idx, &if_data); ++idx)
    {
        DWORD req_size = 0;
        SetupDiGetDeviceInterfaceDetailW(dev_info, &if_data, nullptr, 0, &req_size, nullptr);
        if (req_size == 0)
        {
            continue;
        }

        std::vector<uint8_t> detail_buf(req_size, 0);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(detail_buf.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        if (!SetupDiGetDeviceInterfaceDetailW(dev_info, &if_data, detail, req_size, nullptr, nullptr))
        {
            continue;
        }

        std::string utf8_path = WideToUtf8(detail->DevicePath);
        std::string lower_path = ToLowerAscii(utf8_path);
        if (lower_path.find("vid_1024&pid_2030") == std::string::npos)
        {
            continue;
        }

        HANDLE h = CreateFileW(
            detail->DevicePath,
            0,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr);

        if (h == INVALID_HANDLE_VALUE || h == nullptr)
        {
            continue;
        }

        PHIDP_PREPARSED_DATA preparsed = nullptr;
        HIDP_CAPS caps{};
        if (HidD_GetPreparsedData(h, &preparsed))
        {
            NTSTATUS st = HidP_GetCaps(preparsed, &caps);
            HidD_FreePreparsedData(preparsed);

            if (st == HIDP_STATUS_SUCCESS && caps.FeatureReportByteLength == FEATURE_REPORT_LEN)
            {
                handle_ = h;
                location_ = utf8_path;
                SetupDiDestroyDeviceInfoList(dev_info);
                return true;
            }
        }

        CloseHandle(h);
    }

    SetupDiDestroyDeviceInfoList(dev_info);
    return false;
}

void XanovaMagnetarController::Close()
{
    std::lock_guard<std::mutex> lock(io_mutex_);
    if (handle_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }
    custom_mode_initialized_ = false;
}

bool XanovaMagnetarController::IsConnected() const
{
    std::lock_guard<std::mutex> lock(io_mutex_);
    return handle_ != INVALID_HANDLE_VALUE;
}

std::string XanovaMagnetarController::GetLocation() const
{
    std::lock_guard<std::mutex> lock(io_mutex_);
    return location_;
}

bool XanovaMagnetarController::SetFeatureRaw(uint8_t* report_65)
{
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    return HidD_SetFeature(handle_, report_65, FEATURE_REPORT_LEN) != FALSE;
}

bool XanovaMagnetarController::SendChecksummedCommand(uint8_t b4, uint8_t b5, uint8_t b6, uint8_t* response_65)
{
    if (handle_ == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    uint8_t buf[FEATURE_REPORT_LEN]{};
    buf[4] = b4;
    buf[5] = b5;
    buf[6] = b6;

    uint8_t chk = 0;
    for (size_t i = 3; i < 63; ++i)
    {
        chk ^= buf[i];
    }
    buf[63] = chk;

    bool sent = false;
    for (int attempt = 0; attempt < 20; ++attempt)
    {
        if (HidD_SetFeature(handle_, buf, FEATURE_REPORT_LEN))
        {
            sent = true;
            break;
        }
        Sleep(1);
    }
    if (!sent)
    {
        return false;
    }

    Sleep(5);
    for (int poll = 0; poll < 200; ++poll)
    {
        std::memset(buf, 0, FEATURE_REPORT_LEN);
        if (HidD_GetFeature(handle_, buf, FEATURE_REPORT_LEN))
        {
            if (buf[1] == 0x02 && buf[4] == b4 && buf[5] == b5 && buf[6] == b6)
            {
                if (response_65)
                {
                    std::memcpy(response_65, buf, FEATURE_REPORT_LEN);
                }
                return true;
            }
            if (buf[1] == 0x03 && buf[4] == b4 && buf[5] == b5 && buf[6] == b6)
            {
                return false;
            }
        }
        Sleep(10);
    }

    return false;
}

std::string XanovaMagnetarController::GetFirmwareVersion()
{
    std::lock_guard<std::mutex> lock(io_mutex_);
    uint8_t resp[FEATURE_REPORT_LEN]{};
    if (!SendChecksummedCommand(0x03, 0x00, 0x81, resp))
    {
        return "Unknown";
    }
    char ver[32]{};
    std::snprintf(ver, sizeof(ver), "%u.%u.%u", resp[7], resp[8], resp[9]);
    return std::string(ver);
}

bool XanovaMagnetarController::EnterCustomModeLocked()
{
    uint8_t hdr[FEATURE_REPORT_LEN]{};
    hdr[4] = 0x03;
    hdr[5] = 0x01;
    hdr[6] = 0x00;
    hdr[7] = 0xFE;

    if (!SetFeatureRaw(hdr))
    {
        return false;
    }

    if (!custom_mode_initialized_)
    {
        Sleep(50);
        custom_mode_initialized_ = true;
    }
    return true;
}

bool XanovaMagnetarController::SetLEDMatrix(const uint8_t matrix_rgb[MATRIX_ROWS][MATRIX_COLS][3])
{
    std::lock_guard<std::mutex> lock(io_mutex_);

    if (handle_ == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    if (!EnterCustomModeLocked())
    {
        return false;
    }

    for (size_t ui_row = 0; ui_row < MATRIX_ROWS; ++ui_row)
    {
        uint8_t hw_row = static_cast<uint8_t>((MATRIX_ROWS - 1) - ui_row);

        uint8_t p1[FEATURE_REPORT_LEN]{};
        p1[4] = 0x3A;
        p1[5] = 0x01;
        p1[6] = 0x03;
        p1[7] = 0x06;
        p1[8] = 0x16;
        p1[9] = hw_row;
        p1[10] = 0x00;

        for (size_t c = 0; c < 18; ++c)
        {
            p1[11 + c * 3 + 0] = matrix_rgb[ui_row][c][0];
            p1[11 + c * 3 + 1] = matrix_rgb[ui_row][c][1];
            p1[11 + c * 3 + 2] = matrix_rgb[ui_row][c][2];
        }

        if (!SetFeatureRaw(p1))
        {
            return false;
        }

        uint8_t p2[FEATURE_REPORT_LEN]{};
        p2[4] = 0x10;
        p2[5] = 0x01;
        p2[6] = 0x03;
        p2[7] = 0x06;
        p2[8] = 0x16;
        p2[9] = hw_row;
        p2[10] = 0x12;

        for (size_t c = 0; c < 4; ++c)
        {
            p2[11 + c * 3 + 0] = matrix_rgb[ui_row][18 + c][0];
            p2[11 + c * 3 + 1] = matrix_rgb[ui_row][18 + c][1];
            p2[11 + c * 3 + 2] = matrix_rgb[ui_row][18 + c][2];
        }

        if (!SetFeatureRaw(p2))
        {
            return false;
        }
    }

    return true;
}
