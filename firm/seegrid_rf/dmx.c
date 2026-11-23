#include "dmx.h"
#include "cybsp.h"
#include "cy_utils.h"
#include "seeGridRF.h"

#define TESTING

extern volatile uint32_t msTicks;
static uint32_t timer = 0;

#ifdef TESTING
static void timer_reset() {
    timer = msTicks;
}

static bool timer_expired(const uint16_t period) {
    uint32_t dT = msTicks - timer;
    if (dT > period) {
        return true;
    }
    else return false;
}

/**
 * Test pattern: rotating color wheel
 * universe is filled with fixture frames (layout, width and byte order from seeGridRF.h),
 * hue spread over all fixtures, white off
 */
#define TEST_FRAME_PERIOD_MS    40      /* ~25 frames/s */
#define TEST_CHANNELS           512
#define TEST_FIXTURES           (TEST_CHANNELS / FIXTURE_FRAME_BYTES)
#define TEST_HUE_STEP           4       /* hue advance per frame, 0..1535 per revolution */
#define TEST_CH_MAX             ((1UL << (8 * FIXTURE_CH_BYTES)) - 1)

/* 8 bit -> full fixture channel range */
static sgRF_fixCh_t scale(uint8_t v) {
    return (sgRF_fixCh_t)(v * TEST_CH_MAX / 255);
}

/* hue 0..1535 (6 sectors * 256) -> full saturation/brightness color */
static void hue_to_frame(uint16_t hue, sgRF_fixFrame_t* fix) {
    uint8_t f = hue & 0xFF;
    uint8_t r, g, b;
    switch (hue >> 8) {
        case 0:  r = 255;     g = f;       b = 0;       break;
        case 1:  r = 255 - f; g = 255;     b = 0;       break;
        case 2:  r = 0;       g = 255;     b = f;       break;
        case 3:  r = 0;       g = 255 - f; b = 255;     break;
        case 4:  r = f;       g = 0;       b = 255;     break;
        default: r = 255;     g = 0;       b = 255 - f; break;
    }
    fix->white = 0;
    fix->red   = scale(r);
    fix->green = scale(g);
    fix->blue  = scale(b);
}

static void test_colorwheel(dmx_buf_t buf) {
    static uint16_t hue = 0;
    sgRF_fixFrame_t fix;
    for (uint16_t i = 0; i < TEST_FIXTURES; i++) {
        hue_to_frame((hue + (uint32_t)i * 1536 / TEST_FIXTURES) % 1536, &fix);
        sgRF_writeFrame(&buf[i * FIXTURE_FRAME_BYTES], &fix);
    }
    hue = (hue + TEST_HUE_STEP) % 1536;
}
#endif

/**
 * User Commands
 */
    #ifdef TESTING
    void dmx_init(dmx_t* dmx) {
        timer_reset();
    }
    #else
    void dmx_init(dmx_t* dmx){
        /* configured and initialized via config tool */
    }
    #endif
    
    #ifdef TESTING
    void dmx_handle(dmx_t* dmx){
        if (timer_expired(TEST_FRAME_PERIOD_MS)) {
            test_colorwheel(dmx->buffer);
            dmx->_new_frame = true;
            timer_reset();
        }
    }
    #else
    void dmx_handle(dmx_t* conf) {

    }
    #endif

    bool dmx_newFrame(dmx_t* dmx) {
        return dmx->_new_frame;
    }

    void dmx_clearFrame(dmx_t* dmx) {
        dmx->_new_frame = false;
    }