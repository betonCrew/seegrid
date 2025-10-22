#include "ledLib.h"

#define RGBW

#if defined RGB || defined RGBW
#define RGBW_R_CH 0
#define RGBW_G_CH 1
#define RGBW_B_CH 2
#endif
#ifdef RGBW
#define RGBW_W_CH 3
#endif


static void _led_initChannel(led_channel_t* ch);


void led_init(led_t* led){
    if( led->type == RGBLED || led->type == RGBWLED ) {
        _led_initChannel(led->channel+RGBW_R_CH);
        _led_initChannel(led->channel+RGBW_G_CH);
        _led_initChannel(led->channel+RGBW_B_CH);
    }
    if (led->type == RGBWLED) {
        _led_initChannel(led->channel+RGBW_W_CH);
    }
}


void led_enable(const led_t* led){
    if (led->type == RGBLED || led->type == RGBWLED) {
        XMC_CCU4_SLICE_StartTimer(led->channel[RGBW_R_CH].timerHw);
        XMC_CCU4_SLICE_StartTimer(led->channel[RGBW_G_CH].timerHw);
        XMC_CCU4_SLICE_StartTimer(led->channel[RGBW_B_CH].timerHw);
    }
    if (led->type == RGBWLED) {
        XMC_CCU4_SLICE_StartTimer(led->channel[RGBW_W_CH].timerHw);
    }
}

void led_disable(const led_t* led) {
    if (led->type == RGBLED || led->type == RGBWLED) {
        XMC_CCU4_SLICE_StopTimer(led->channel[RGBW_R_CH].timerHw);
        XMC_CCU4_SLICE_StopTimer(led->channel[RGBW_G_CH].timerHw);
        XMC_CCU4_SLICE_StopTimer(led->channel[RGBW_B_CH].timerHw);
    }
    if (led->type == RGBWLED) {
        XMC_CCU4_SLICE_StopTimer(led->channel[RGBW_W_CH].timerHw);
    }
}

void led_setColorPreset(const led_t* led, const color_preset_t preset){
        XMC_CCU4_SLICE_SetTimerCompareMatch(led->channel[RGBW_R_CH].timerHw, color_presetGetRed(preset));
        XMC_CCU4_SLICE_SetTimerCompareMatch(led->channel[RGBW_G_CH].timerHw, color_presetGetGreen(preset));
        XMC_CCU4_SLICE_SetTimerCompareMatch(led->channel[RGBW_B_CH].timerHw, color_presetGetBlue(preset));
        XMC_CCU4_SLICE_SetTimerCompareMatch(led->channel[RGBW_B_CH].timerHw, color_presetGetWhite(preset));
        XMC_CCU4_EnableShadowTransfer(PWM_GEN_HW, XMC_CCU4_SHADOW_TRANSFER_SLICE_0);
        XMC_CCU4_EnableShadowTransfer(PWM_GEN_HW, XMC_CCU4_SHADOW_TRANSFER_SLICE_1);
        XMC_CCU4_EnableShadowTransfer(PWM_GEN_HW, XMC_CCU4_SHADOW_TRANSFER_SLICE_2);
        XMC_CCU4_EnableShadowTransfer(PWM_GEN_HW, XMC_CCU4_SHADOW_TRANSFER_SLICE_3);
}

void led_setColor(const led_t* led, const led_color_t col){
    if (led->type == RGBLED || led->type == RGBWLED) {
        XMC_CCU4_SLICE_SetTimerCompareMatch(led->channel[RGBW_R_CH].timerHw, col.red);
        XMC_CCU4_SLICE_SetTimerCompareMatch(led->channel[RGBW_G_CH].timerHw, col.green);
        XMC_CCU4_SLICE_SetTimerCompareMatch(led->channel[RGBW_B_CH].timerHw, col.blue);
        // FIXME use multi shadow transfer!
        XMC_CCU4_EnableShadowTransfer(PWM_GEN_HW, XMC_CCU4_SHADOW_TRANSFER_SLICE_0);
        XMC_CCU4_EnableShadowTransfer(PWM_GEN_HW, XMC_CCU4_SHADOW_TRANSFER_SLICE_1);
        XMC_CCU4_EnableShadowTransfer(PWM_GEN_HW, XMC_CCU4_SHADOW_TRANSFER_SLICE_2);
    }
}


void led_setWhite(const led_t* led, const uint16_t val){
    if (led->type == RGBWLED) {
        XMC_CCU4_SLICE_SetTimerCompareMatch(led->channel[RGBW_W_CH].timerHw, val);
        XMC_CCU4_EnableShadowTransfer(PWM_GEN_HW, XMC_CCU4_SHADOW_TRANSFER_SLICE_3 );

    }
}


static void _led_initChannel(led_channel_t* ch) {
    /**
     * Timer Setup done via config tool!
     */
    // TODO check Timer config regarding pwm frequency
}