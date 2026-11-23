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
    #define DMX_RDM_MAX     257     /* RDM packet incl. start code and checksum */

    /* Enttec DMX USB Pro labels, widget -> host */
    #define DMX_LABEL_RECEIVED      5   /* received packet (here: RDM response) */
    #define DMX_LABEL_RDM_TIMEOUT   12  /* no RDM response */

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

        /* RDM request (label 7, discovery: label 11), one at a time for the main loop */
        bool            _rdm_keep;
        uint8_t         _rdm_data[DMX_RDM_MAX];
        uint16_t        _rdm_len;
        volatile bool   _rdm_discovery;
        volatile bool   _rdm_ready;

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
     * pending RDM request from the host; release it with dmx_rdmDone
     */
    bool dmx_rdmRequest(dmx_t* dmx, const uint8_t** data, uint16_t* len, bool* discovery);
    void dmx_rdmDone(dmx_t* dmx);

    /**
     * send an Enttec packet to the host (blocking, ~10 us per byte)
     */
    void dmx_send(dmx_t* dmx, const uint8_t label, const uint8_t* data, const uint16_t len);

    /**
     * true, if there is a new frame
     */
    bool dmx_newFrame(dmx_t* dmx);

    /**
     * reset the new frame flag
     */
    void dmx_clearFrame(dmx_t* dmx);

#endif