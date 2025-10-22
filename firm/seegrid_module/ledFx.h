#ifndef LEDFX_H
#define LEDFX_H

#include "colors.h"
#include "ledLib.h"


typedef enum {
    FX_OFF,             // off
    FX_SOLID,
    FX_SINE,
    FX_SINE_RAND,       // dim down with sine value
    FX_PULSE_SLOW,      // aus dark aufleuchten in dark abdimmen
    FX_PULSE_FAST,
    FX_SPARKLE,
    FX_FLSH_OVR_CLR,
    FX_COL_WH_BL,
    FX_COL_WH_PU,       // subtle color change with fade between
    FX_COL_CY_PI,
    FX_RAMP,            // ramp up and quickly fall down
    FX_COUNT
} ledFx_t;


/*
    Palette
    COL_WHITE, COL_BLUE_LIGHT
    COL_WHITE, COL_PURPLE_DARK,
    COL_CYAN, COL_PINK
*/


void ledFx_set(ledFx_t fx, color_preset_t col);
void ledFx_run(led_t* led, const ledFx_t fx, const color_preset_t fx_col);



#endif