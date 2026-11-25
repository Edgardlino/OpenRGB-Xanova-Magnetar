#include "RGBController_XanovaMagnetar.h"
#include "RGBControllerKeyNames.h"

#include <cstring>

namespace
{
    constexpr unsigned int NA = 0xFFFFFFFFU;
    constexpr unsigned int MATRIX_ROWS = 6;
    constexpr unsigned int MATRIX_COLS = 22;

    // 107 physical keys in UI row order (row 0 = top ESC row, row 5 = bottom CTRL row)
    struct MagnetarKeyDef
    {
        unsigned int ui_row;
        unsigned int col;
        const char*  name;
    };

    const MagnetarKeyDef kMagnetarABNT2Keys[] =
    {
        // Row 0 (hw_row 5): ESC + F-keys + Nav top
        {0,  0, KEY_EN_ESCAPE},
        {0,  1, KEY_EN_F1},
        {0,  2, KEY_EN_F2},
        {0,  3, KEY_EN_F3},
        {0,  4, KEY_EN_F4},
        {0,  6, KEY_EN_F5},
        {0,  7, KEY_EN_F6},
        {0,  8, KEY_EN_F7},
        {0,  9, KEY_EN_F8},
        {0, 11, KEY_EN_F9},
        {0, 12, KEY_EN_F10},
        {0, 13, KEY_EN_F11},
        {0, 14, KEY_EN_F12},
        {0, 15, KEY_EN_PRINT_SCREEN},
        {0, 16, KEY_EN_SCROLL_LOCK},
        {0, 17, KEY_EN_PAUSE_BREAK},

        // Row 1 (hw_row 4): Number row + Nav + Numpad top
        {1,  0, KEY_EN_QUOTE},
        {1,  1, KEY_EN_1},
        {1,  2, KEY_EN_2},
        {1,  3, KEY_EN_3},
        {1,  4, KEY_EN_4},
        {1,  5, KEY_EN_5},
        {1,  6, KEY_EN_6},
        {1,  7, KEY_EN_7},
        {1,  8, KEY_EN_8},
        {1,  9, KEY_EN_9},
        {1, 10, KEY_EN_0},
        {1, 11, KEY_EN_MINUS},
        {1, 12, KEY_EN_EQUALS},
        {1, 14, KEY_EN_BACKSPACE},
        {1, 15, KEY_EN_INSERT},
        {1, 16, KEY_EN_HOME},
        {1, 17, KEY_EN_PAGE_UP},
        {1, 18, KEY_EN_NUMPAD_LOCK},
        {1, 19, KEY_EN_NUMPAD_DIVIDE},
        {1, 20, KEY_EN_NUMPAD_TIMES},
        {1, 21, KEY_EN_NUMPAD_MINUS},

        // Row 2 (hw_row 3): Tab / QWERTY + Nav + Numpad 7-9/+
        {2,  0, KEY_EN_TAB},
        {2,  1, KEY_EN_Q},
        {2,  2, KEY_EN_W},
        {2,  3, KEY_EN_E},
        {2,  4, KEY_EN_R},
        {2,  5, KEY_EN_T},
        {2,  6, KEY_EN_Y},
        {2,  7, KEY_EN_U},
        {2,  8, KEY_EN_I},
        {2,  9, KEY_EN_O},
        {2, 10, KEY_EN_P},
        {2, 11, KEY_ES_TILDE},
        {2, 12, KEY_EN_LEFT_BRACKET},
        {2, 15, KEY_EN_DELETE},
        {2, 16, KEY_EN_END},
        {2, 17, KEY_EN_PAGE_DOWN},
        {2, 18, KEY_EN_NUMPAD_7},
        {2, 19, KEY_EN_NUMPAD_8},
        {2, 20, KEY_EN_NUMPAD_9},
        {2, 21, KEY_EN_NUMPAD_PLUS},

        // Row 3 (hw_row 2): Caps / ASDF + Ç + ~ + ] + ISO Enter + Numpad 4-6/.
        {3,  0, KEY_EN_CAPS_LOCK},
        {3,  1, KEY_EN_A},
        {3,  2, KEY_EN_S},
        {3,  3, KEY_EN_D},
        {3,  4, KEY_EN_F},
        {3,  5, KEY_EN_G},
        {3,  6, KEY_EN_H},
        {3,  7, KEY_EN_J},
        {3,  8, KEY_EN_K},
        {3,  9, KEY_EN_L},
        {3, 10, KEY_FR_CEDILLA_C},
        {3, 11, KEY_BR_TILDE},
        {3, 12, KEY_EN_RIGHT_BRACKET},
        {3, 13, KEY_EN_ISO_ENTER},
        {3, 18, KEY_EN_NUMPAD_4},
        {3, 19, KEY_EN_NUMPAD_5},
        {3, 20, KEY_EN_NUMPAD_6},
        {3, 21, KEY_EN_NUMPAD_PERIOD},

        // Row 4 (hw_row 1): LShift + \ (ABNT2) + ZXCV + /? + RShift + Up + Numpad 1-3/Enter
        {4,  0, KEY_EN_LEFT_SHIFT},
        {4,  1, KEY_EN_ISO_BACK_SLASH},
        {4,  2, KEY_EN_Z},
        {4,  3, KEY_EN_X},
        {4,  4, KEY_EN_C},
        {4,  5, KEY_EN_V},
        {4,  6, KEY_EN_B},
        {4,  7, KEY_EN_N},
        {4,  8, KEY_EN_M},
        {4,  9, KEY_EN_COMMA},
        {4, 10, KEY_EN_PERIOD},
        {4, 11, KEY_EN_SEMICOLON},
        {4, 12, KEY_EN_FORWARD_SLASH},
        {4, 13, KEY_EN_RIGHT_SHIFT},
        {4, 16, KEY_EN_UP_ARROW},
        {4, 18, KEY_EN_NUMPAD_1},
        {4, 19, KEY_EN_NUMPAD_2},
        {4, 20, KEY_EN_NUMPAD_3},
        {4, 21, KEY_EN_NUMPAD_ENTER},

        // Row 5 (hw_row 0): Bottom CTRL row
        {5,  0, KEY_EN_LEFT_CONTROL},
        {5,  1, KEY_EN_LEFT_WINDOWS},
        {5,  2, KEY_EN_LEFT_ALT},
        {5,  4, KEY_EN_SPACE},
        {5,  9, KEY_EN_RIGHT_ALT},
        {5, 10, KEY_EN_RIGHT_FUNCTION},
        {5, 11, KEY_EN_MENU},
        {5, 12, KEY_EN_RIGHT_CONTROL},
        {5, 15, KEY_EN_LEFT_ARROW},
        {5, 16, KEY_EN_DOWN_ARROW},
        {5, 17, KEY_EN_RIGHT_ARROW},
        {5, 19, KEY_EN_NUMPAD_0},
        {5, 21, "Key: Number Pad ,"},
    };
}

RGBController_XanovaMagnetar::RGBController_XanovaMagnetar(XanovaMagnetarController* controller_ptr)
    : controller(controller_ptr)
{
    name        = "Xanova Magnetar RGB";
    vendor      = "Xanova";
    type        = DEVICE_TYPE_KEYBOARD;
    description = "Xanova Magnetar RGB Mechanical Keyboard (ABNT2)";
    version     = controller->GetFirmwareVersion();
    location    = controller->GetLocation();
    flags       = CONTROLLER_FLAG_LOCAL;

    mode Direct;
    Direct.name       = "Direct";
    Direct.value      = 0xFE;
    Direct.flags      = MODE_FLAG_HAS_PER_LED_COLOR;
    Direct.color_mode = MODE_COLORS_PER_LED;
    modes.push_back(Direct);

    SetupZones();
}

RGBController_XanovaMagnetar::~RGBController_XanovaMagnetar()
{
    for (zone& z : zones)
    {
        if (z.matrix_map)
        {
            delete[] z.matrix_map->map;
            delete z.matrix_map;
            z.matrix_map = nullptr;
        }
    }
    delete controller;
    controller = nullptr;
}

void RGBController_XanovaMagnetar::SetupZones()
{
    constexpr unsigned int key_count = sizeof(kMagnetarABNT2Keys) / sizeof(kMagnetarABNT2Keys[0]);

    zone kbd_zone;
    kbd_zone.name       = ZONE_EN_KEYBOARD;
    kbd_zone.type       = ZONE_TYPE_MATRIX;
    kbd_zone.leds_min   = key_count;
    kbd_zone.leds_max   = key_count;
    kbd_zone.leds_count = key_count;
    kbd_zone.matrix_map = new matrix_map_type;
    kbd_zone.matrix_map->height = MATRIX_ROWS;
    kbd_zone.matrix_map->width  = MATRIX_COLS;
    kbd_zone.matrix_map->map    = new unsigned int[MATRIX_ROWS * MATRIX_COLS];

    std::fill(kbd_zone.matrix_map->map, kbd_zone.matrix_map->map + (MATRIX_ROWS * MATRIX_COLS), NA);

    leds.clear();
    leds.reserve(key_count);

    for (unsigned int i = 0; i < key_count; ++i)
    {
        const MagnetarKeyDef& k = kMagnetarABNT2Keys[i];
        led new_led;
        new_led.name  = k.name;
        new_led.value = (k.ui_row << 8) | k.col;
        leds.push_back(new_led);

        kbd_zone.matrix_map->map[k.ui_row * MATRIX_COLS + k.col] = i;
    }

    zones.clear();
    zones.push_back(kbd_zone);

    SetupColors();
}

void RGBController_XanovaMagnetar::ResizeZone(int /*zone*/, int /*new_size*/)
{
}

void RGBController_XanovaMagnetar::DeviceUpdateLEDs()
{
    uint8_t matrix_rgb[MATRIX_ROWS][MATRIX_COLS][3]{};

    for (size_t i = 0; i < leds.size() && i < colors.size(); ++i)
    {
        unsigned int row = (leds[i].value >> 8) & 0xFF;
        unsigned int col = leds[i].value & 0xFF;
        if (row < MATRIX_ROWS && col < MATRIX_COLS)
        {
            RGBColor c = colors[i];
            matrix_rgb[row][col][0] = static_cast<uint8_t>(RGBGetRValue(c));
            matrix_rgb[row][col][1] = static_cast<uint8_t>(RGBGetGValue(c));
            matrix_rgb[row][col][2] = static_cast<uint8_t>(RGBGetBValue(c));
        }
    }

    controller->SetLEDMatrix(matrix_rgb);
}

void RGBController_XanovaMagnetar::UpdateZoneLEDs(int /*zone*/)
{
    DeviceUpdateLEDs();
}

void RGBController_XanovaMagnetar::UpdateSingleLED(int /*led*/)
{
    DeviceUpdateLEDs();
}

void RGBController_XanovaMagnetar::DeviceUpdateMode()
{
}
