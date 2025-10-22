#include "colors.h"



#define UPSCALE_COL(x) (x << 8 | x);

uint8_t color_presets[COL_COUNT*4] = {
    0x00, 0x00, 0x00, 0x00,             // BLACK
    0x00, 0x00, 0x00, 0xFF,             // WHITE
    0x30, 0x00, 0x00, 0xFF,             // CANDLELIGHT
    0x00, 0xBC, 0xFE, 0x00,             // BLUE Light
    0x11, 0x11, 0xEE, 0x00,             // BLUE Dark
    0x24, 0xE4, 0xFD, 0x00,             // CYAN
    0xFD, 0x24, 0xFA, 0x00,             // PINK
    0xD9, 0x00, 0xFF, 0x00,             // PURPLE
    0xFF, 0x00, 0x00, 0x00              // RED
};

// FIXME change return type to led_val_t
inline const uint16_t color_presetGetRed(const color_preset_t preset){
    return UPSCALE_COL(color_presets[preset*4+0]);
}
inline const uint16_t color_presetGetGreen(const color_preset_t preset){
    return UPSCALE_COL(color_presets[preset*4+1]);
}
inline const uint16_t color_presetGetBlue(const color_preset_t preset){
    return UPSCALE_COL(color_presets[preset*4+2]);
}
inline const uint16_t color_presetGetWhite(const color_preset_t preset){
    return UPSCALE_COL(color_presets[preset*4+3]);
}