#include "math.h"
#include "ledFx.h"
#include "ledLib.h"
#include "colors.h"

/* math */
#define M_PI		3.14159265358979323846
#define M_PI_2		1.57079632679489661923

/* SysTick*/
extern volatile uint32_t msTicks;

/* fx engine statics */
static ledFx_t _fx = 0;
static color_preset_t _col = COL_BLACK;
static uint8_t _phase = 0;
static uint32_t _phaseStart = 0;

static void _run_FX_OFF(led_t* led) {
    led_setColorPreset(led, COL_BLACK);
}

static void _run_FX_SOLID(led_t* led, const color_preset_t fx_col) {
    led_setColorPreset(led, fx_col);
}

static void _run_FX_SINE(led_t* led, const color_preset_t fx_col) {
    const uint32_t periodMs = 50000;
    const float periodAngle = 2*M_PI;
    static uint32_t periodStart;
    const led_color_t led_col = {
        color_presetGetRed(fx_col),
        color_presetGetGreen(fx_col),
        color_presetGetBlue(fx_col)};
    const led_white_t led_white = color_presetGetWhite(fx_col);

    uint32_t timeNow = msTicks;
    uint32_t timePassed = timeNow - periodStart;
    if (timePassed > periodMs) periodStart = timeNow;         
    float x = sin(periodAngle / periodMs * timePassed);
    if (x < 0) led_setColorPreset(led, COL_BLACK);
    else {
        led_setColor(led, (led_color_t){
        x*led_col.red, x*led_col.green, x*led_col.blue});
        led_setWhite(led, x*led_white);
    }
}
static void _run_FX_SINE_RAND() {
    // TODO
}

/**
 * aus dark aufleuchten, in dark abdimmen, kürzer halten als aus
 *              ...
 *             .   .
 *            .     .
 *           .       .
 *          .         ..............
 */
static void _run_FX_PULSE_SLOW() {
    // TODO

    const uint32_t times[] = {2000, 2000, 2000, 4000};
    const uint16_t vals[] = {0xFFFF, 0xFFFF, 0x0000, 0x0000};

    switch(_phase) {
        case 0:
            if (msTicks-_phaseStart >= times[0]) _phase = 1;

            break;
        case 1:
            if (msTicks-_phaseStart >= times[1]) _phase = 2;
            // TODO hold
            break;
        case 2:
            if (msTicks-_phaseStart >= times[2]) _phase = 3;
            break;
        case 3:
            if (msTicks-_phaseStart >= times[3]) _phase = 0;
            break;
    }


}
static void _run_FX_PULSE_FAST() {

}
static void _run_FX_SPARKLE() {
    // TODO
}
static void _run_FX_COL_WH_BL() {

}
static void _run_FX_COL_WH_PU() {
    // TODO
}
static void _run_FX_COL_CY_PI() {

}
static void _run_FX_RAMP() {
    // TODO
}


void ledFx_set(ledFx_t fx, color_preset_t col) {
    _fx = fx;
    _col = col;
    // TODO reset state
    _phase = 0;
    _phaseStart = msTicks;
}

void ledFx_run(led_t* led, const ledFx_t fx, const color_preset_t fx_col) {
    
    switch(fx){
        case FX_SOLID: _run_FX_SOLID(led, fx_col); break;
        case FX_SINE: _run_FX_SINE(led, fx_col); break;
        default: _run_FX_OFF(led); break;
    }
}

// void ledFx_run() {
//     switch(_fx) {
//         case FX_SOLID:
//             _run_FX_SOLID();
//             break;
//         case FX_SINE:
//             _run_FX_SINE();
//             break;
//         case FX_SINE_RAND:
//             _run_FX_SINE_RAND();
//             break;
//         case FX_PULSE_SLOW:
//             _run_FX_PULSE_SLOW();
//             break;
//         case FX_PULSE_FAST:
//             _run_FX_PULSE_FAST();
//             break;
//         case FX_SPARKLE:
//             _run_FX_SPARKLE();
//             break;
//         case FX_COL_WH_BL:
//             _run_FX_COL_WH_BL();
//             break;
//         case FX_COL_WH_PU:
//             _run_FX_COL_WH_PU();
//             break;
//         case FX_COL_CY_PI:
//             _run_FX_COL_CY_PI();
//             break;
//         case FX_RAMP:
//             _run_FX_RAMP();
//             break;
//         default:
//              // TODO handle fx off
//              break;
//     }
// }
