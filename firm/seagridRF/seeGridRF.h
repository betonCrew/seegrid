#ifndef SEEGRIDRF_H
#define SEEGRIDRF_H

#include "nrf24l01.h"
#include "cybsp.h"
#include "cy_utils.h"

/**
 * This file is used to define the rf protocol
 * used to transmit light control data over air
 */


/**
 * Fixture related configurations
 */

    #define FIXTURE_CH_CNT      4
    #define FIXTURE_CH_BYTES    1

    #define ADDR_WIDTH          3
    #define AUTO_RETRANS        0

    #define CRC_EN              0
    #define CRC_CODE            0

    #define RF_POWER            0b11

    /* ShockBurst */
    #define AUTO_ACK            0
    #define RX_ADDR             0

/**
 * RF Protocol Definitions
 */

    #define FRAME_ADDR_WIDTH    3
    #define FRAME_ADDR_BASE     (0b10101010 << 16 | 0b10101010 << 8)

    #define BROADCAST_CH        125
    #define BROADCAST_DATARATE  R_1Mbps
    #define BROADCAST_POWER     P_0dBM
    #define BROADCAST_RX_PIPE   0
    #define BROADCAST_PL_WITDH  32

    #define CONTROL_CH          64
    #define CONTROL_DATARATE    R_250kbps
    #define CONTROL_POWER       P_0dBM


/**
 * Type Definitions
 */


    #if (FIXTURE_CH_BYTES == 1)
        typedef uint8_t sgRF_fixCh_t;
    #elif (FIXTURE_CH_BYTES == 2)
        typedef uint16_t sgRF_fixCh_t;
    #endif

    #if (FIXTURE_CH_CNT == 4)
        typedef struct {
            sgRF_fixCh_t white;
            sgRF_fixCh_t red;
            sgRF_fixCh_t blue;
            sgRF_fixCh_t green;
        } sgRF_fixFrame_t;
    #endif

    typedef enum {
    OFF, STBY,
    GW_TRANS, GW_BROADCAST,
    FIX_BROADCAST, FIX_CONTROL
            } _sgRF_state_t;

    typedef enum {FIXTURE, GATEWAY} sgRF_mode_t;

    typedef uint8_t _sgRF_dmx_addr_t;

    typedef uint8_t _sgRF_dmx_width_t;

    typedef struct {
        XMC_USIC_t *const       usic;
        XMC_USIC_CH_t *const    spiHardware;
        uint8_t const           spiSelo;
        IRQn_Type               irqN;
        XMC_GPIO_PORT_t*        irqPort;
        uint8_t                 irqPin;
        XMC_GPIO_PORT_t*        cePort;
        uint8_t                 cePin;
        sgRF_mode_t             mode;
        uint8_t*                data;
        _sgRF_state_t           _state;
        _sgRF_dmx_addr_t        _addr;
        _sgRF_dmx_width_t       _width;
        volatile bool           _tx_flag;
        volatile bool           _rx_flag;
        volatile uint8_t        _rx_pipe;
        volatile bool           _rt_flag;
        bool                    _frame_flag;
    } sgRF_radio_t;


/**
 * User Commands
 */

    /**
     * initialize the given hardware as seagrifRF radio!
     */
    void sgRF_init(sgRF_radio_t* radio);
    void sgRF_handle(sgRF_radio_t* radio);
    void sgRF_radioIrqHandler(sgRF_radio_t* radio);

    /* Controlling */

        /**
         * this will turn on controller mode
         */
        void sgRF_enableController();

    /* DMX Transmitting*/

        /**
         * check if radio is ready to go into broadcast transmit (STBY)
         */
        bool sgRF_readyToBroadcast(sgRF_radio_t* radio);

        /**
         * this will turn on broadcast transmitter mode
         */
        void sgRF_enableBroadcastTransmitter(sgRF_radio_t* radio);

        /**
         * this will turn off broadcast transmitter mode
         */
        void sgRF_disableBroadcastTransmitter(sgRF_radio_t* radio);

        /**
         * Broadcast a 512b wide universe once!
         */
        void sgRF_broadcastDmxFrame(const sgRF_radio_t* radio, const uint8_t* frame);

    /* DMX Receiving */
    
        /**
         * this will turn on receiving mode
         */
        void sgRF_enableBroadcastReceiver(sgRF_radio_t* radio);

        /**
         * this will turn of receiving mode
         */
        void sgRF_disableBroadcastReceiver(sgRF_radio_t* radio);

        /**
         * check, if a new DmxFrame has been received and can be read from buffer
         */
        bool sgRF_receivedFrame(const sgRF_radio_t* radio);

        /**
         * return last frame from radio buffer
         */
        const sgRF_fixFrame_t sgRF_readFrame(sgRF_radio_t* radio);


#endif