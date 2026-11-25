#pragma once

#include <cstdint>
#include <mutex>
#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

class XanovaMagnetarController
{
public:
    static constexpr uint16_t VID = 0x1024;
    static constexpr uint16_t PID = 0x2030;
    static constexpr size_t FEATURE_REPORT_LEN = 65;
    static constexpr size_t MATRIX_ROWS = 6;
    static constexpr size_t MATRIX_COLS = 22;

    XanovaMagnetarController();
    ~XanovaMagnetarController();

    bool Open();
    void Close();
    bool IsConnected() const;

    std::string GetLocation() const;
    std::string GetFirmwareVersion();

    /**
     * Send a 6x22 RGB matrix in UI order (row 0 = top ESC row, row 5 = bottom CTRL row).
     * Internally flips rows to match hardware order (hw_row = 5 - ui_row).
     */
    bool SetLEDMatrix(const uint8_t matrix_rgb[MATRIX_ROWS][MATRIX_COLS][3]);

private:
    bool SetFeatureRaw(uint8_t* report_65);
    bool SendChecksummedCommand(uint8_t b4, uint8_t b5, uint8_t b6, uint8_t* response_65);
    bool EnterCustomModeLocked();

    HANDLE handle_;
    std::string location_;
    bool custom_mode_initialized_;
    mutable std::mutex io_mutex_;
};
