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

    /**
     * 4 colour channels W R B G; the personality (RDM DMX_PERSONALITY) sets
     * their width, chosen per fixture:
     *  1: 8 bit  -> 4 DMX channels
     *  2: 16 bit -> 8 DMX channels, high byte first (DMX coarse/fine), default
     */
    #define FIXTURE_CH_CNT              4
    #define SGRF_PERSONALITY_8BIT       1
    #define SGRF_PERSONALITY_16BIT      2
    #define SGRF_PERSONALITY_COUNT      2
    #define SGRF_PERSONALITY_DEFAULT    SGRF_PERSONALITY_16BIT

    #define ADDR_WIDTH          3
    #define AUTO_RETRANS        0


    #define RF_POWER            0b11

    /* ShockBurst */
    #define AUTO_ACK            0
    #define RX_ADDR             0

/**
 * RF Protocol Definitions
 */

    #define FRAME_ADDR_WIDTH    3
    /* upper address bytes, the LSByte selects the chunk (see seeGridRF.c).
       Must not continue the preamble (nRF24L01+ spec 7.3.2) */
    #define FRAME_ADDR_BASE     (0x057D << 8)

    #define BROADCAST_CH        125
    #define BROADCAST_DATARATE  R_1Mbps
    #define BROADCAST_POWER     P_0dBM
    #define BROADCAST_RX_PIPE   0
    #define BROADCAST_PL_WITDH  32
    #define BROADCAST_CRC       CRC_2B

    /* 2480 MHz, inside the ISM band and outside the busy WiFi channels here.
       2 Mbps: the module's carrier sits ~0.5-1 MHz off the gateway's (far
       beyond the +-60 ppm crystal spec); 250 kbps got nothing through, 1 Mbps
       24-80 %, 2 Mbps (twice the deviation) all of it */
    #define CONTROL_CH          80
    #define CONTROL_DATARATE    R_2Mbps
    #define CONTROL_POWER       P_0dBM

    /**
     * Control packets, 16 bytes, CRC 2 bytes, no ACK:
     *  - heartbeat: fixture -> gateway on CONTROL_CH (gateway radio B, pipe 0)
     *  - config:    gateway -> fixture on BROADCAST_CH after the DMX frame
     *               (fixture pipe 1, next to the DMX chunk on pipe 0)
     * Addresses avoid preamble-like runs and differ from the chunk addresses.
     */
    #define CONTROL_PL_WIDTH    16
    #define HEARTBEAT_ADDR      0x13C6B4UL
    #define CONFIG_ADDR         0x0E9C4BUL
    #define CONFIG_RX_PIPE      1

    #define SGRF_PKT_HEARTBEAT  0x01
    #define SGRF_PKT_CONFIG     0x02

    /* heartbeat flags */
    #define SGRF_HB_IDENTIFY    0x01    /* identify (blinking) active */
    #define SGRF_HB_NO_SIGNAL   0x02    /* no DMX frame for SIGNAL_TIMEOUT */

    /* config fields meaning "leave as is" */
    #define SGRF_CFG_KEEP_ADDR          0x0000
    #define SGRF_CFG_KEEP_IDENTIFY      0xFF
    #define SGRF_CFG_KEEP_PERSONALITY   0x00


/**
 * Type Definitions
 */


    /* channel value, always on the 16 bit scale (8 bit values are scaled up) */
    typedef uint16_t sgRF_fixCh_t;

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
    GW_TRANS, GW_BROADCAST, GW_STATUS,
    FIX_BROADCAST, FIX_CONTROL
            } _sgRF_state_t;

    typedef enum {FIXTURE, GATEWAY} sgRF_mode_t;

    /* DMX channel offset 0..511 (start address - 1) */
    typedef uint16_t _sgRF_dmx_addr_t;

    typedef struct __attribute__((packed)) {
        uint8_t     type;           /* SGRF_PKT_HEARTBEAT */
        uint8_t     seq;
        uint32_t    id;             /* fixture id (CRC32 of the chip UCID) */
        uint16_t    dmx_addr;       /* DMX start address 1..512 */
        uint8_t     personality;
        uint8_t     flags;          /* SGRF_HB_* */
        uint16_t    battery_mv;
        uint8_t     rx_fps;         /* DMX frames received during the last second */
        uint8_t     fw_version;
        uint8_t     reserved[2];
    } sgRF_heartbeat_t;

    typedef struct __attribute__((packed)) {
        uint8_t     type;           /* SGRF_PKT_CONFIG */
        uint8_t     seq;
        uint32_t    id;             /* target fixture */
        uint16_t    dmx_addr;       /* new start address, SGRF_CFG_KEEP_ADDR = keep */
        uint8_t     identify;       /* 0 off, 1 on, SGRF_CFG_KEEP_IDENTIFY = keep */
        uint8_t     personality;    /* SGRF_PERSONALITY_*, SGRF_CFG_KEEP_PERSONALITY = keep */
        uint8_t     reserved[6];
    } sgRF_config_t;

    _Static_assert(sizeof(sgRF_heartbeat_t) == CONTROL_PL_WIDTH, "heartbeat size");
    _Static_assert(sizeof(sgRF_config_t) == CONTROL_PL_WIDTH, "config size");


    typedef struct {
        XMC_USIC_t *const       usic;
        XMC_USIC_CH_t *const    spiHardware;
        XMC_SPI_CH_SLAVE_SELECT_t const spiSelo;
        IRQn_Type               irqN;
        XMC_GPIO_PORT_t*        irqPort;
        uint8_t                 irqPin;
        XMC_GPIO_PORT_t*        cePort;
        uint8_t                 cePin;
        sgRF_mode_t             mode;
        uint8_t*                data;
        _sgRF_state_t           _state;
        _sgRF_dmx_addr_t        _addr;
        volatile bool           _irq_pending;
        volatile bool           _tx_flag;
        volatile bool           _rx_flag;
        volatile uint8_t        _rx_pipe;
        volatile bool           _rt_flag;
        bool                    _frame_flag;
        bool                    _config_flag;
        sgRF_config_t           _config;
    } sgRF_radio_t;


/**
 * User Commands
 */

    /**
     * initialize the given hardware as seagridRF radio!
     * returns false (state OFF) if no nRF answers on the bus
     */
    bool sgRF_init(sgRF_radio_t* radio);
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

        /**
         * Broadcast a config packet (after the DMX frame, broadcast transmitter active)
         */
        void sgRF_broadcastConfig(const sgRF_radio_t* radio, const sgRF_config_t* config);

    /* Status receiving (gateway) */

        /**
         * listen for fixture heartbeats on the control channel
         */
        void sgRF_enableStatusReceiver(sgRF_radio_t* radio);

        /**
         * fetch the next received heartbeat; false if there is none
         */
        bool sgRF_readHeartbeat(sgRF_radio_t* radio, sgRF_heartbeat_t* heartbeat);

    /* DMX Receiving */
    
        /**
         * this will turn on receiving mode (DMX chunk of the DMX address + config packets)
         */
        void sgRF_enableBroadcastReceiver(sgRF_radio_t* radio);

        /**
         * a fixture listens to one 32 byte chunk: its channels must not cross a
         * chunk boundary. dmx_addr = start address 1..512
         */
        bool sgRF_dmxAddressValid(const uint16_t dmx_addr, const uint8_t footprint);

        /**
         * set the DMX channel offset (start address - 1) the fixture listens to
         */
        void sgRF_setDmxAddress(sgRF_radio_t* radio, const _sgRF_dmx_addr_t offset);

        /**
         * send a heartbeat on the control channel (~1 ms, DMX reception paused)
         */
        void sgRF_sendHeartbeat(sgRF_radio_t* radio, const sgRF_heartbeat_t* heartbeat);

        /**
         * check for / fetch a received config packet
         */
        bool sgRF_receivedConfig(const sgRF_radio_t* radio);
        sgRF_config_t sgRF_readConfig(sgRF_radio_t* radio);

        /**
         * this will turn of receiving mode
         */
        void sgRF_disableBroadcastReceiver(sgRF_radio_t* radio);

        /**
         * on-air address of the packet carrying the given 32 byte chunk
         */
        uint32_t sgRF_chunkAddress(const uint8_t chunk);

        /**
         * check, if a new DmxFrame has been received and can be read from buffer
         */
        bool sgRF_receivedFrame(const sgRF_radio_t* radio);

        /**
         * return last frame from radio buffer
         */
        const sgRF_fixFrame_t sgRF_readFrame(sgRF_radio_t* radio, const uint8_t personality);

        /**
         * encode a fixture frame into its DMX byte layout (sgRF_footprint bytes at dst)
         */
        void sgRF_writeFrame(uint8_t* dst, const sgRF_fixFrame_t* frame, const uint8_t personality);

        /**
         * DMX channels of a fixture in the given personality, 0 if there is no such personality
         */
        uint8_t sgRF_footprint(const uint8_t personality);


#endif