#ifndef DMX_H
#define DMX_H

#include "cybsp.h"
#include "cy_utils.h"
#include "stdint.h"

/**
 * Type Definitions
 */

    typedef uint8_t* dmx_buf_t;

    #define DMX_CHANNELS    512

    typedef enum {
        _DMX_RX_SOM, _DMX_RX_LABEL, _DMX_RX_LEN_LO, _DMX_RX_LEN_HI, _DMX_RX_DATA, _DMX_RX_EOM
    } _dmx_rx_state_t;

    typedef struct {
        XMC_USIC_CH_t*  usic_ch;
        IRQn_Type       irqN;
        dmx_buf_t       buffer;
        bool            _new_frame;

        /* Enttec DMX USB Pro receiver, filled from the UART interrupt */
        _dmx_rx_state_t _rx_state;
        uint8_t         _rx_label;
        uint16_t        _rx_len;
        uint16_t        _rx_pos;
        bool            _rx_keep;                   /* store this packet's data */
        uint8_t         _rx_data[2][1 + DMX_CHANNELS]; /* start code + channels, double buffered */
        uint16_t        _rx_channels[2];
        uint8_t         _rx_fill;                   /* buffer the interrupt writes into */
        volatile bool   _rx_ready;                  /* the other buffer holds a new frame */

        /* statistics */
        volatile uint32_t frames;
        volatile uint32_t errors;
    } dmx_t;


/**
 * User Commands
 */

    void dmx_init(dmx_t* dmx);
    void dmx_handle(dmx_t* dmx);

    /**
     * call from the UART receive (FIFO standard) interrupt
     */
    void dmx_rxIrqHandler(dmx_t* dmx);

    /**
     * true, if there is a new frame
     */
    bool dmx_newFrame(dmx_t* dmx);

    /**
     * reset the new frame flag
     */
    void dmx_clearFrame(dmx_t* dmx);

#endif