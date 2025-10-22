#ifndef DMX_H
#define DMX_H

#include "cybsp.h"
#include "cy_utils.h"
#include "stdint.h"

/**
 * Type Definitions
 */

    typedef uint8_t* dmx_buf_t;

    typedef struct {
        XMC_USIC_CH_t*  usic_ch;
        dmx_buf_t       buffer;
        bool            _new_frame;
    } dmx_t;


/**
 * User Commands
 */

    void dmx_init(dmx_t* dmx);
    void dmx_handle(dmx_t* dmx);

    /**
     * true, if there is a new frame
     */
    bool dmx_newFrame(dmx_t* dmx);

    /**
     * reset the new frame flag
     */
    void dmx_clearFrame(dmx_t* dmx);

#endif