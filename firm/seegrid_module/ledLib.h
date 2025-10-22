#ifndef LEDLIB_H
#define LEDLIB_H

#include "cybsp.h"
#include "stdbool.h"
#include "stdint.h"
#include "colors.h"

/**
 * Library for PWM control of multicolor LEDs
 */


typedef enum {RGBLED, RGBWLED} led_type_t;

typedef uint16_t led_val_t;

typedef struct {
    CCU4_CC4_TypeDef*   timerHw;
    XMC_GPIO_PORT_t*    port;
    uint8_t             pin;
    bool                init;
} led_channel_t;

typedef struct {
    led_type_t          type;
    led_channel_t       channel[4];
} led_t;

typedef struct {
    led_val_t           red;
    led_val_t           green;
    led_val_t           blue;
} led_color_t;

typedef struct {
    uint16_t            stepTime;
    led_val_t*          data;
} led_sequence_t;

typedef led_val_t led_white_t;

void led_init(led_t* led);
void led_enable(const led_t* led);
void led_disable(const led_t* led);
void led_setColorPreset(const led_t* led, const color_preset_t preset);
void led_setColor(const led_t* led, const led_color_t col);
void led_setWhite(const led_t* led, const led_white_t);
void led_playSequence(const led_t* led, const led_sequence_t* sequence);
void led_pauseSequence(const led_t* led, const led_sequence_t* sequence);
void led_resetSequence(const led_t* led, const led_sequence_t* sequence);

#endif