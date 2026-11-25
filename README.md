# OpenRGB Support for Xanova Magnetar RGB

OpenRGB support for the **Xanova Magnetar RGB Mechanical Keyboard** (`VID 0x1024 / PID 0x2030`, Galax/Xanova — ABNT2 107-key layout), reverse-engineered from `DeviceDll.dll` and validated on real hardware (`FW 1.14.4`).

> **Português:** Suporte ao teclado mecânico **Xanova Magnetar RGB (ABNT2, `1024:2030`)** no **OpenRGB**, incluindo plugin C++ nativo (Windows), controlador multiplataforma pronto para contribuir ao OpenRGB oficial e uma ponte leve via E1.31.

---

## Repository Structure

- **`plugin/`** — Native OpenRGB C++ Plugin (`OpenRGBPluginInterface` API v4, Qt 5.15, Windows MSVC x64). Compiles directly via GitHub Actions (`.github/workflows/build.yml`) into `XanovaMagnetarPlugin.dll`.
- **`upstream/Controllers/XanovaMagnetarController/`** — Cross-platform OpenRGB controller (`<hidapi.h>`, `SPDX-License-Identifier: GPL-2.0-only`, `REGISTER_HID_DETECTOR_IPU`) ready to drop into `Controllers/` of a `CalcProgrammer1/OpenRGB` fork for a Merge Request.
- **`e131-bridge/`** — Lightweight Python E1.31 (sACN) → USB HID bridge (`xanova_magnetar_e131_bridge.py` + `start_bridge.bat`) that works with **any** OpenRGB version without compiling C++.

---

## Option 1: Install the OpenRGB Plugin (`.dll` via GitHub Actions)

Compatible with **OpenRGB `v0.9+ / 1.0rc2`** (`OPENRGB_PLUGIN_API_VERSION = 4`, Windows x64 MSVC + Qt 5.15).

1. Go to the **Actions** tab of this repository and open the latest **Build XanovaMagnetarPlugin (Windows MSVC x64)** run (or click **Run workflow**).
2. Download the **`XanovaMagnetarPlugin-win64`** artifact and extract `XanovaMagnetarPlugin.dll`.
3. Close **OpenRGB** and **XANOVA CONTROL PANEL** (if running).
4. Copy `XanovaMagnetarPlugin.dll` to:
   ```text
   %APPDATA%\OpenRGB\plugins\
   ```
5. Launch OpenRGB. **Xanova Magnetar RGB** will appear under **Devices** with all 107 ABNT2 keys in **Direct** mode.
   *(If you plug the keyboard in later, open **Settings → Xanova Magnetar** and click **Re-detectar Xanova Magnetar**.)*

---

## Option 2: E1.31 Bridge (Works on Any OpenRGB Version, No Build Needed)

1. Close **XANOVA CONTROL PANEL**.
2. Run `e131-bridge/start_bridge.bat` (requires Python 3 on Windows).
3. In OpenRGB, go to **Settings → E1.31 Devices → Add** and configure:
   - **Name:** `Xanova Magnetar RGB`
   - **IP:** `127.0.0.1`
   - **Type:** `MATRIX` (`22` width × `6` height, `Horizontal Top Left`, `132` LEDs)
   - **Start Universe:** `1` | **Start Channel:** `1` | **Universe Size:** `510`
   - **RGB Order:** `RGB` | **Keepalive Time:** `1000`
4. Click **Save** and **Rescan Devices**.

---

## Option 3: Upstream OpenRGB Contribution (Windows / Linux / macOS)

1. Fork `https://gitlab.com/CalcProgrammer1/OpenRGB`.
2. Copy `upstream/Controllers/XanovaMagnetarController/` into `Controllers/XanovaMagnetarController/` in your fork (`OpenRGB.pro` auto-discovers `Controllers/*.h` and `Controllers/*.cpp`).
3. On Linux, regenerate udev rules so `1024:2030` is accessible without root.
4. Test via the OpenRGB GitLab CI pipeline and open a Merge Request.

---

## Reverse-Engineered USB HID Protocol Summary

- **USB IDs:** `VID = 0x1024`, `PID = 0x2030`
- **HID Control Interface:** `MI_02` (`interface = 2`, `UsagePage = 0x0001`, `Usage = 0x0002`, `FeatureReportByteLength = 65`)
- **Report format:** 65-byte HID Feature Reports (`Report ID = 0x00`).
- **Query commands** (e.g. `GetFWVersion` `03 00 81`, `GetDeviceLayout` `01 00 85`):
  - XOR checksum of bytes `3..62` stored at byte `63`.
  - Poll `HidD_GetFeature` until byte `1 == 0x02` (`0x01` = busy, `0x03` = error) matching echoed command bytes `4..6`.
- **Direct / Per-Key LED Matrix (`SetCustomLEDEffect`):**
  1. Send header `00 00 00 00 03 01 00 FE ...` (wait ~50 ms on initial mode transition).
  2. Send 12 Feature Reports covering 6 rows × 22 columns (hardware row order is bottom-to-top: `hw_row 0` = bottom `Ctrl` row, `hw_row 5` = top `Esc` row):
     - Packet 1 (`18` LEDs, cols `0..17`): `00 00 00 00 3A 01 03 06 16 <hw_row> 00 <18×RGB>`
     - Packet 2 (`4` LEDs, cols `18..21`): `00 00 00 00 10 01 03 06 16 <hw_row> 12 <4×RGB>`

### Validated 107-Key ABNT2 Matrix (`hw_row 0..5`)
- `hw_row 5` (UI row 0 — ESC): cols `0..4, 6..9, 11..17` (16 keys)
- `hw_row 4` (UI row 1 — Numbers): cols `0..12, 14..21` (21 keys)
- `hw_row 3` (UI row 2 — Tab): cols `0..12, 15..21` (20 keys)
- `hw_row 2` (UI row 3 — Caps): cols `0..13` (`13` = ISO Enter), `18..21` (`21` = Numpad `.`) (18 keys)
- `hw_row 1` (UI row 4 — Shift): cols `0..13` (`13` = Right Shift), `16`, `18..21` (19 keys)
- `hw_row 0` (UI row 5 — Ctrl): cols `0, 1, 2, 4, 9, 10, 11, 12, 15, 16, 17, 19, 21` (13 keys)

## License

GPL-2.0-only (see [LICENSE](LICENSE)), matching the OpenRGB project.
