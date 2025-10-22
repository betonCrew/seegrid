#ifndef COLORS_H
#define COLORS_H

#include "stdint.h"


typedef enum {
    COL_BLACK,
    COL_WHITE,
    COL_CANDLELIGTH,
    COL_BLUE_LIGHT,
    COL_BLUE_DARK,
    COL_CYAN,
    COL_PINK,
    COL_PURPLE,
    COL_RED,
    COL_COUNT
} color_preset_t;


const uint16_t color_presetGetRed(const color_preset_t preset);
const uint16_t color_presetGetGreen(const color_preset_t preset);
const uint16_t color_presetGetBlue(const color_preset_t preset);
const uint16_t color_presetGetWhite(const color_preset_t preset);





#endif