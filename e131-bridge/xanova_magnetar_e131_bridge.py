"""
OpenRGB E1.31 (sACN) -> Xanova Magnetar RGB (VID 0x1024 / PID 0x2030) Bridge.

Listens for E1.31 Universe 1 (22x6 RGB matrix, HORIZONTAL_TOP_LEFT, 396 channels)
and forwards frames via Windows HID Feature Reports (MI_02, 65-byte reports)
using the reverse-engineered DeviceDll.dll protocol.
"""

import argparse
import ctypes
from ctypes import wintypes
import socket
import struct
import threading
import time

VID_MATCH = "vid_1024&pid_2030"
FEATURE_REPORT_LEN = 65
MATRIX_ROWS = 6
MATRIX_COLS = 22
NUM_LEDS = MATRIX_ROWS * MATRIX_COLS  # 132
NUM_CHANNELS = NUM_LEDS * 3           # 396

# Physical 107-key ABNT2 mask in hardware row order (row 0 = bottom CTRL, row 5 = top ESC)
ABNT2_KEY_MASK = {
    5: [0, 1, 2, 3, 4, 6, 7, 8, 9, 11, 12, 13, 14, 15, 16, 17],
    4: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 18, 19, 20, 21],
    3: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 16, 17, 18, 19, 20, 21],
    2: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 18, 19, 20, 21],
    1: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 18, 19, 20, 21],
    0: [0, 1, 2, 4, 9, 10, 11, 12, 15, 16, 17, 19, 21],
}

# Win32 HID / SetupAPI bindings
hid = ctypes.windll.hid
setupapi = ctypes.windll.setupapi
kernel32 = ctypes.windll.kernel32


class GUID(ctypes.Structure):
    _fields_ = [
        ("Data1", wintypes.DWORD),
        ("Data2", wintypes.WORD),
        ("Data3", wintypes.WORD),
        ("Data4", ctypes.c_ubyte * 8),
    ]


class SP_DEVICE_INTERFACE_DATA(ctypes.Structure):
    _fields_ = [
        ("cbSize", wintypes.DWORD),
        ("InterfaceClassGuid", GUID),
        ("Flags", wintypes.DWORD),
        ("Reserved", ctypes.POINTER(ctypes.c_ulong)),
    ]


class HIDP_CAPS(ctypes.Structure):
    _fields_ = [
        ("Usage", wintypes.USHORT),
        ("UsagePage", wintypes.USHORT),
        ("InputReportByteLength", wintypes.USHORT),
        ("OutputReportByteLength", wintypes.USHORT),
        ("FeatureReportByteLength", wintypes.USHORT),
        ("Reserved", wintypes.USHORT * 17),
        ("Extra", wintypes.USHORT * 10),
    ]


setupapi.SetupDiGetClassDevsW.restype = ctypes.c_void_p
setupapi.SetupDiEnumDeviceInterfaces.argtypes = [
    ctypes.c_void_p,
    ctypes.c_void_p,
    ctypes.POINTER(GUID),
    wintypes.DWORD,
    ctypes.POINTER(SP_DEVICE_INTERFACE_DATA),
]
setupapi.SetupDiGetDeviceInterfaceDetailW.argtypes = [
    ctypes.c_void_p,
    ctypes.POINTER(SP_DEVICE_INTERFACE_DATA),
    ctypes.c_void_p,
    wintypes.DWORD,
    ctypes.POINTER(wintypes.DWORD),
    ctypes.c_void_p,
]
setupapi.SetupDiDestroyDeviceInfoList.argtypes = [ctypes.c_void_p]
kernel32.CreateFileW.restype = ctypes.c_void_p
kernel32.CloseHandle.argtypes = [ctypes.c_void_p]
hid.HidD_GetPreparsedData.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p)]
hid.HidP_GetCaps.argtypes = [ctypes.c_void_p, ctypes.POINTER(HIDP_CAPS)]
hid.HidD_FreePreparsedData.argtypes = [ctypes.c_void_p]
hid.HidD_SetFeature.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.ULONG]

INVALID_HANDLE = ctypes.c_void_p(-1).value
ACN_PACKET_ID = b"ASC-E1.17\x00\x00\x00"


class XanovaMagnetarHID:
    """Manages the Windows HID Feature Report handle for the Xanova Magnetar."""

    def __init__(self) -> None:
        self.handle = None
        self.initialized_custom = False

    def open(self) -> bool:
        self.close()
        guid = GUID()
        hid.HidD_GetHidGuid(ctypes.byref(guid))
        hdev = setupapi.SetupDiGetClassDevsW(ctypes.byref(guid), None, None, 0x12)
        if not hdev or hdev == INVALID_HANDLE:
            return False

        try:
            did = SP_DEVICE_INTERFACE_DATA()
            did.cbSize = ctypes.sizeof(SP_DEVICE_INTERFACE_DATA)
            idx = 0
            detail_offset = ctypes.sizeof(wintypes.DWORD)
            cb_size = 8 if ctypes.sizeof(ctypes.c_void_p) == 8 else 6

            while setupapi.SetupDiEnumDeviceInterfaces(hdev, None, ctypes.byref(guid), idx, ctypes.byref(did)):
                idx += 1
                req = wintypes.DWORD()
                setupapi.SetupDiGetDeviceInterfaceDetailW(hdev, ctypes.byref(did), None, 0, ctypes.byref(req), None)
                if req.value == 0:
                    continue
                buf = ctypes.create_string_buffer(req.value)
                ctypes.cast(buf, ctypes.POINTER(wintypes.DWORD))[0] = cb_size
                if not setupapi.SetupDiGetDeviceInterfaceDetailW(
                    hdev, ctypes.byref(did), buf, req.value, None, None
                ):
                    continue
                path = ctypes.wstring_at(ctypes.addressof(buf) + detail_offset)
                if VID_MATCH not in path.lower():
                    continue

                h = kernel32.CreateFileW(path, 0, 3, None, 3, 0, None)
                if h == INVALID_HANDLE or h == 0:
                    continue

                pp = ctypes.c_void_p()
                caps = HIDP_CAPS()
                if hid.HidD_GetPreparsedData(h, ctypes.byref(pp)):
                    hid.HidP_GetCaps(pp, ctypes.byref(caps))
                    hid.HidD_FreePreparsedData(pp)
                    if caps.FeatureReportByteLength == FEATURE_REPORT_LEN:
                        self.handle = h
                        self.initialized_custom = False
                        return True
                kernel32.CloseHandle(h)
        finally:
            setupapi.SetupDiDestroyDeviceInfoList(hdev)
        return False

    def close(self) -> None:
        if self.handle is not None:
            kernel32.CloseHandle(self.handle)
            self.handle = None
            self.initialized_custom = False

    def _set_feature(self, buf: bytearray) -> bool:
        if self.handle is None:
            return False
        raw = (ctypes.c_ubyte * FEATURE_REPORT_LEN).from_buffer(buf)
        return bool(hid.HidD_SetFeature(self.handle, raw, FEATURE_REPORT_LEN))

    def enter_custom_mode(self) -> bool:
        hdr = bytearray(FEATURE_REPORT_LEN)
        hdr[4:8] = b"\x03\x01\x00\xfe"
        if not self._set_feature(hdr):
            return False
        time.sleep(0.05)
        self.initialized_custom = True
        return True

    def send_frame(self, dmx_bytes: bytes, mask_unused: bool = True, send_header: bool = True) -> bool:
        if self.handle is None and not self.open():
            return False

        if not self.initialized_custom:
            if not self.enter_custom_mode():
                self.close()
                return False
        elif send_header:
            hdr = bytearray(FEATURE_REPORT_LEN)
            hdr[4:8] = b"\x03\x01\x00\xfe"
            if not self._set_feature(hdr):
                self.close()
                return False

        # OpenRGB E1.31 MATRIX (HORIZONTAL_TOP_LEFT): ui_row 0 = top (ESC), ui_row 5 = bottom (CTRL).
        # Hardware protocol expects hw_row 0 = bottom (CTRL), hw_row 5 = top (ESC).
        for ui_row in range(MATRIX_ROWS):
            hw_row = (MATRIX_ROWS - 1) - ui_row
            row_start = ui_row * MATRIX_COLS * 3

            p1 = bytearray(FEATURE_REPORT_LEN)
            p1[4:11] = bytes((0x3A, 0x01, 0x03, 0x06, 0x16, hw_row, 0x00))

            p2 = bytearray(FEATURE_REPORT_LEN)
            p2[4:11] = bytes((0x10, 0x01, 0x03, 0x06, 0x16, hw_row, 0x12))

            valid_cols = ABNT2_KEY_MASK[hw_row] if mask_unused else None

            for c in range(18):
                if valid_cols is not None and c not in valid_cols:
                    continue
                off = row_start + c * 3
                p1[11 + c * 3 : 14 + c * 3] = dmx_bytes[off : off + 3]

            for c in range(4):
                col = 18 + c
                if valid_cols is not None and col not in valid_cols:
                    continue
                off = row_start + col * 3
                p2[11 + c * 3 : 14 + c * 3] = dmx_bytes[off : off + 3]

            if not self._set_feature(p1):
                self.close()
                return False
            if not self._set_feature(p2):
                self.close()
                return False

        return True


def parse_e131_packet(data: bytes, target_universe: int, start_channel: int) -> bytes | None:
    """Validate an E1.31 sACN data packet and return the 396 RGB channel bytes."""
    if len(data) < 126:
        return None
    if data[0:2] != b"\x00\x10" or data[4:16] != ACN_PACKET_ID:
        return None
    if struct.unpack_from(">I", data, 18)[0] != 0x00000004:
        return None
    if struct.unpack_from(">I", data, 40)[0] != 0x00000002:
        return None

    universe = struct.unpack_from(">H", data, 113)[0]
    if universe != target_universe:
        return None

    if data[117] != 0x02 or data[125] != 0x00:
        return None

    prop_count = struct.unpack_from(">H", data, 123)[0]
    if prop_count <= 1:
        return None

    dmx_payload = data[126 : 126 + (prop_count - 1)]
    offset = start_channel - 1
    if len(dmx_payload) < offset + NUM_CHANNELS:
        padded = bytearray(NUM_CHANNELS)
        avail = max(0, len(dmx_payload) - offset)
        if avail > 0:
            padded[:avail] = dmx_payload[offset : offset + avail]
        return bytes(padded)
    return dmx_payload[offset : offset + NUM_CHANNELS]


def main() -> None:
    parser = argparse.ArgumentParser(description="OpenRGB E1.31 to Xanova Magnetar HID bridge")
    parser.add_argument("--bind", default="127.0.0.1", help="IP to bind UDP socket (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=5568, help="E1.31 UDP port (default: 5568)")
    parser.add_argument("--universe", type=int, default=1, help="E1.31 universe (default: 1)")
    parser.add_argument("--start-channel", type=int, default=1, help="DMX start channel (default: 1)")
    parser.add_argument("--max-fps", type=float, default=60.0, help="Max HID refresh rate in FPS (default: 60)")
    parser.add_argument("--no-frame-header", action="store_true", help="Skip 03 01 00 FE header after first frame")
    parser.add_argument("--no-mask", action="store_true", help="Send all 132 cells instead of masking 107 ABNT2 keys")
    parser.add_argument("--test-color", help="Send a single R,G,B test frame and exit (e.g. 0,180,255)")
    args = parser.parse_args()

    kbd = XanovaMagnetarHID()
    if kbd.open():
        print("[Magnetar] Teclado conectado (VID_1024&PID_2030).")
    else:
        print("[Magnetar] Teclado nao encontrado agora; aguardando reconexao...")

    if args.test_color:
        r, g, b = (int(x) for x in args.test_color.split(","))
        frame = bytes((r, g, b)) * NUM_LEDS
        ok = kbd.send_frame(frame, mask_unused=not args.no_mask, send_header=not args.no_frame_header)
        print(f"[Magnetar] Test frame ({r},{g},{b}) sent: {ok}")
        kbd.close()
        return

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((args.bind, args.port))
    print(f"[E1.31] Escutando em {args.bind}:{args.port} (Universe {args.universe}, 22x6 = {NUM_LEDS} LEDs)...")

    latest_lock = threading.Lock()
    latest_frame: list[bytes | None] = [None]
    frame_event = threading.Event()
    stop_event = threading.Event()

    def hid_worker() -> None:
        min_interval = 1.0 / max(1.0, args.max_fps)
        last_sent = 0.0
        last_payload: bytes | None = None
        while not stop_event.is_set():
            if not frame_event.wait(timeout=0.5):
                continue
            frame_event.clear()
            with latest_lock:
                payload = latest_frame[0]
            if payload is None or payload == last_payload:
                continue

            now = time.monotonic()
            wait = min_interval - (now - last_sent)
            if wait > 0:
                time.sleep(wait)
                with latest_lock:
                    payload = latest_frame[0] or payload

            if kbd.send_frame(payload, mask_unused=not args.no_mask, send_header=not args.no_frame_header):
                last_payload = payload
                last_sent = time.monotonic()
            else:
                time.sleep(1.0)

    worker = threading.Thread(target=hid_worker, daemon=True)
    worker.start()

    try:
        while True:
            data, _ = sock.recvfrom(2048)
            frame = parse_e131_packet(data, args.universe, args.start_channel)
            if frame is not None:
                with latest_lock:
                    latest_frame[0] = frame
                frame_event.set()
    except KeyboardInterrupt:
        print("\n[Magnetar] Encerrando ponte...")
    finally:
        stop_event.set()
        frame_event.set()
        worker.join(timeout=1.0)
        kbd.close()
        sock.close()


if __name__ == "__main__":
    main()
