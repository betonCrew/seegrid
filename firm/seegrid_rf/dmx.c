#include "dmx.h"
#include "cybsp.h"
#include "cy_utils.h"

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
        if (timer_expired(1000)) {
            // TODO write test data into universe
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