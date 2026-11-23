#include "cybsp.h"
#include "cy_utils.h"
#include "seeGridRF.h"
#include "nrf24l01.h"


#ifdef DEBUG
typedef struct {
    uint32_t    tx_count;
    uint32_t    rx_count;
    uint32_t    loop_count;
    uint8_t     nrf24_status;
} debug_t;

extern volatile debug_t debug;
#endif


/**
 * Private
 */

    /**
     * Payload Configuration
     * 
     *  defines how much bytes of payload are needed for the fixture
     *  and how many channels are encoded within it
     */
    typedef uint8_t _sgRF_payloadConf_t;
    /* 5bits payload size -> 0..31Byte */
    #define PAYLOAD_CONF_SIZE
    #define PAYLOAD_CONF_SIZE_BIT   0
    #define PAYLOAD_CONF_SIZE_MSK   0b11111
    /* 3bits channel count -> 0..7 channels */
    #define PAYLOAD_CONF_CH
    #define PAYLOAD_CONF_CH_BIT     5
    #define PAYLOAD_CONF_CH_MSK     0b111

    typedef struct{
        uint32_t ucid[4];
        uint8_t batLvl;
        _sgRF_payloadConf_t paylodConf;
        /* last 2 byte reserved */
    } _sgRF_fixtureIdentifier_t;

    /**
     *  LSByte of the packet address per 32 byte chunk: the 16 codewords of
     *  the extended Hamming code [8,4,4] XOR 0x08. Every two chunk addresses
     *  differ in >= 4 bits, no byte has a run longer than 4 bits, and with
     *  FRAME_ADDR_BASE no address continues the preamble by more than 4 bits
     *  (nRF24L01+ spec 7.3.2: preamble-like addresses raise packet errors)
     */
    static const uint8_t _chunk_addr_lookup_table[512/BROADCAST_PL_WITDH] = {
        0x08, 0x16, 0x25, 0x3B, 0x43, 0x5D, 0x6E, 0x70,
        0x8F, 0x91, 0xA2, 0xBC, 0xC4, 0xDA, 0xE9, 0xF7
    };

    uint32_t sgRF_chunkAddress(const uint8_t chunk) {
        return FRAME_ADDR_BASE | _chunk_addr_lookup_table[chunk % (512/BROADCAST_PL_WITDH)];
    }


bool sgRF_init(sgRF_radio_t* radio){

    nrf24_init(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, radio->usic);
    /* enable irq system of xmc! */
    __enable_irq();

    XMC_DelayUs(500);

    nrf24_reset(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin);
    /* after reset the status reads 0x0E; MISO stuck high (0xFF) = no radio */
    if (nrf24_readStatus(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin) != 0x0E) {
        radio->_state = OFF;
        return false;
    }

    nrf24_powerUp(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin);
    /* Tpd2stby: up to 1.5ms until standby-I is reached */
    XMC_Delay(2);
    radio->_state = STBY;
    return true;
}

/**
 * RF State Machine
 */

    static bool _sgRF_setState(sgRF_radio_t* radio, const _sgRF_state_t state) {
        if(radio->_state != STBY) return false;
        radio->_state = state;
        return true;
    }

    static void _sgRF_resetState(sgRF_radio_t* radio) {
        radio->_state = STBY;
        return;
    }

/**
 * Controlling
 */

/**
 * DMX Transmitting
 */

    /**
     * User
     */
        
    /**
     * Wait until the TX FIFO is empty, i.e. queued packets are on air.
     * Bound: 3 FIFO slots * ~420us airtime + margin.
     */
    static void _sgRF_waitTxDone(const sgRF_radio_t* radio) {
        for (uint8_t i = 0; i < 40; i++) {
            if (nrf24_txFifoEmpty(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin)) return;
            XMC_DelayUs(50);
        }
    }

    bool sgRF_readyToBroadcast(sgRF_radio_t* radio) {
        if (radio->_state == STBY) return true;
        else return false;
    }

    void sgRF_enableBroadcastTransmitter(sgRF_radio_t* radio) {

        /* check if ready / set state */
        if(!_sgRF_setState(radio, GW_BROADCAST)) return;

        nrf24_setupFlags(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, TX_DS_MASK);
        nrf24_clearFlags(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, RX_DR_MASK|TX_DS_MASK|MAX_RT_MASK);
        nrf24_disableAutoAck(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, 0xFF);
        nrf24_setCrc(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, BROADCAST_CRC);
        nrf24_disableRxAddr(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, 0xFF);
        nrf24_setAddrWidth(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, W_3BYTES);
        nrf24_disableAutoRetransmit(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin);

        nrf24_setChannel(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, BROADCAST_CH);
        nrf24_setDataRate(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, BROADCAST_DATARATE);
        nrf24_setPowerLevel(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, BROADCAST_POWER);

        nrf24_setPrimTx(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin);


        NVIC_SetPriority(radio->irqN, 0);
        NVIC_ClearPendingIRQ(radio->irqN);
        NVIC_EnableIRQ(radio->irqN);
    }

    void sgRF_disableBroadcastTransmitter(sgRF_radio_t* radio) {

        /* check if BroadcastReceiver is active */
        if (radio->_state != GW_BROADCAST) return;

        /* let the last packet go out, so its TX_DS still reaches the IRQ handler */
        _sgRF_waitTxDone(radio);
        /* anything left never got a CE pulse: drop it */
        nrf24_flushTx(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin);

        XMC_GPIO_SetOutputLow(radio->cePort, radio->cePin);
        NVIC_DisableIRQ(radio->irqN);
        _sgRF_resetState(radio);
    }

    void sgRF_broadcastDmxFrame(const sgRF_radio_t* radio, const uint8_t* frame) {
        
        /* check if broadcast transmitter is active*/
        if (radio->_state != GW_BROADCAST) return;

        const uint8_t chunks = 512/BROADCAST_PL_WITDH;
        uint8_t dmx_chunk = 0;
        uint32_t packet_addr;
        /* chunk pointer points to chunk-beginn in dmx universe*/
        const uint8_t* chunk_ptr;
        
        nrf24_flushTx(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin);

        while (dmx_chunk < chunks) {
            packet_addr = sgRF_chunkAddress(dmx_chunk);
            chunk_ptr = frame + (dmx_chunk * BROADCAST_PL_WITDH);
            /* TX_ADDR is applied when a packet goes on air: never change it
               while the previous one is still queued */
            _sgRF_waitTxDone(radio);
            nrf24_setTxAddr(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, packet_addr);
            nrf24_writeTx(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, (char*)chunk_ptr, BROADCAST_PL_WITDH);
            /* pulse carrier enable */
            XMC_GPIO_SetOutputHigh(radio->cePort, radio->cePin);
            XMC_DelayUs(15);
            XMC_GPIO_SetOutputLow(radio->cePort, radio->cePin);
            dmx_chunk++;
        }
    }

/**
 * DMX Receiving
 * 
 *      when radio has detected a signal on broadcast receive pipe,
 *      an interrupt has to set a newFrame-flag!
 */

    /**
     * Internals
     */

    static inline void _sgRF_radioIrqHandler_Rx(sgRF_radio_t* radio, const uint8_t pipe) {
        /* 0b110 not used, 0b111 RxFifo empty */
        if(pipe<0b110) {
            /* set a software flag */
            radio->_rx_flag = true;
            radio->_rx_pipe = pipe;
        }
    }

    static inline void _sgRF_radioIrqHandler_Tx(sgRF_radio_t* radio) {
        radio->_tx_flag = true;
    }

    static inline void _sgRF_radioIqrHandler_Rt(sgRF_radio_t* radio) {
        radio->_rt_flag = true;
    }

    /**
     * ISR: no SPI here, the bus may be busy with a main loop transaction.
     * The flags are fetched and cleared in sgRF_handle.
     */
    void sgRF_radioIrqHandler(sgRF_radio_t* radio) {
        radio->_irq_pending = true;
    }

    /**
     * Read and clear the nRF status flags. The ERU triggers on the falling
     * edge of the active-low IRQ pin, which only rises again once all flags
     * are cleared, so repeat while the pin is still low (a flag set between
     * read and clear would otherwise block every further interrupt).
     */
    static void _sgRF_serviceIrq(sgRF_radio_t* radio) {
        if (!radio->_irq_pending) return;
        radio->_irq_pending = false;

        /* bounded: a missing/hung radio must not block the main loop */
        uint8_t tries = 4;
        do {
            const uint8_t irq_flags = nrf24_readStatus(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin);
            nrf24_clearFlags(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, irq_flags);

            /* Data Ready RX FiFo */
            if (irq_flags & RX_DR_MASK) {
                _sgRF_radioIrqHandler_Rx(radio, (RX_P_NO_MASK&irq_flags)>>RX_P_NO_BIT);
            }

            /* Data Sent TX FiFo */
            if (irq_flags & TX_DS_MASK) {
                _sgRF_radioIrqHandler_Tx(radio);
            }

            /* Max Transmit Retries reached */
            if (irq_flags & MAX_RT_MASK) {
                _sgRF_radioIqrHandler_Rt(radio);
            }
        } while (--tries && XMC_GPIO_GetInput(radio->irqPort, radio->irqPin) == 0);
    }

    static inline uint32_t _sgRF_translateAddress(const _sgRF_dmx_addr_t dmx_addr) {
        /* each chunk holds 32 bytes of dmx data / addresses */
        return sgRF_chunkAddress(dmx_addr / BROADCAST_PL_WITDH);
    }



    /**
     * User
     */

    void sgRF_enableBroadcastReceiver(sgRF_radio_t* radio) {

        /* check if initialized / set state */
        if(!_sgRF_setState(radio, FIX_BROADCAST)) return;

        // TODO where to get the dmx address??
        radio->_addr = 0x00;
        radio->_width = FIXTURE_CH_CNT;
        const uint32_t dmx_chunk_addr = _sgRF_translateAddress(radio->_addr);

        /* config radio channels */
        nrf24_setupFlags(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, RX_DR_MASK);
        nrf24_clearFlags(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, RX_DR_MASK|TX_DS_MASK|MAX_RT_MASK);
        nrf24_disableAutoAck(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, 0xFF);
        nrf24_setCrc(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, BROADCAST_CRC);
        nrf24_setAddrWidth(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, W_3BYTES);
        nrf24_disableAutoRetransmit(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin);

        nrf24_setChannel(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, BROADCAST_CH);
        nrf24_setDataRate(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, BROADCAST_DATARATE);
        nrf24_setPowerLevel(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, BROADCAST_POWER);

        nrf24_setRxAddr(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, BROADCAST_RX_PIPE, dmx_chunk_addr);
        nrf24_enableRxAddr(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, 1<<BROADCAST_RX_PIPE);
        nrf24_setPayloadWidth(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, BROADCAST_RX_PIPE, BROADCAST_PL_WITDH);
        /* after nrf24_reset the radio is PTX: with CE high it would never listen */
        nrf24_setPrimRx(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin);

        /* enable interrupt handling */
        NVIC_SetPriority(radio->irqN, 0);
        NVIC_ClearPendingIRQ(radio->irqN);
        NVIC_EnableIRQ(radio->irqN);
        // TODO set intterupt handler to _sgRF_radioIrqHandler(radio);


        XMC_GPIO_SetOutputHigh(radio->cePort, radio->cePin);
        radio->_state = FIX_BROADCAST;
    }

    void sgRF_disableBroadcastReceiver(sgRF_radio_t* radio) {

        /* check if BroadcastReceiver is active */
        if (radio->_state != FIX_BROADCAST) return;

        XMC_GPIO_SetOutputLow(radio->cePort, radio->cePin);
        NVIC_DisableIRQ(radio->irqN);
        _sgRF_resetState(radio);
    }

    bool sgRF_receivedFrame(const sgRF_radio_t* radio) {
        return radio->_frame_flag;
    }

    /* DMX convention: multi-byte channels are sent coarse (high byte) first */
    static sgRF_fixCh_t _sgRF_getCh(const uint8_t* src) {
        sgRF_fixCh_t val = 0;
        for (uint8_t i = 0; i < FIXTURE_CH_BYTES; i++) {
            val = (sgRF_fixCh_t)((val << 8) | src[i]);
        }
        return val;
    }

    static void _sgRF_putCh(uint8_t* dst, sgRF_fixCh_t val) {
        for (uint8_t i = FIXTURE_CH_BYTES; i > 0; i--) {
            dst[i - 1] = (uint8_t)val;
            val = (sgRF_fixCh_t)(val >> 8);
        }
    }

    const sgRF_fixFrame_t sgRF_readFrame(sgRF_radio_t* radio) {
        const uint8_t* src = radio->data + (radio->_addr % 32);
        radio->_frame_flag = false;
        sgRF_fixFrame_t frame = {
            .white = _sgRF_getCh(src + 0 * FIXTURE_CH_BYTES),
            .red   = _sgRF_getCh(src + 1 * FIXTURE_CH_BYTES),
            .blue  = _sgRF_getCh(src + 2 * FIXTURE_CH_BYTES),
            .green = _sgRF_getCh(src + 3 * FIXTURE_CH_BYTES),
        };
        return frame;
    }

    void sgRF_writeFrame(uint8_t* dst, const sgRF_fixFrame_t* frame) {
        _sgRF_putCh(dst + 0 * FIXTURE_CH_BYTES, frame->white);
        _sgRF_putCh(dst + 1 * FIXTURE_CH_BYTES, frame->red);
        _sgRF_putCh(dst + 2 * FIXTURE_CH_BYTES, frame->blue);
        _sgRF_putCh(dst + 3 * FIXTURE_CH_BYTES, frame->green);
    }

    static void _sgRF_handleFixture(sgRF_radio_t* radio){
        switch(radio->_state) {
            /**
             * STBY
             *      the radio is in standby and available for config changes
             */
            case STBY:
                break;
            /**
             * FIX_CONTROL
             *      the radio is in receive mode in stby for control frame
             */
            case FIX_CONTROL:
                break;
            /**
             * FIX_BROADCAST
             *      the radio is in receive mode for dmx frame 
             */
            case FIX_BROADCAST:
                /* check for packet */
                if(radio->_rx_flag && (radio->_rx_pipe==BROADCAST_RX_PIPE)) {
                    
                    #ifdef DEBUG
                    debug.rx_count++;
                    #endif
                    
                    nrf24_readRx(radio->spiHardware, radio->spiSelo, radio->cePort, radio->cePin, (char*)radio->data, 1, BROADCAST_PL_WITDH);

                    radio->_rx_flag = false;
                    radio->_frame_flag = true;
                }
                // TODO how long do we wait in this mode for a packet? should there be some kind of timeout?
                break;
            /**
             * OFF
             *      probably unintialized, do nothing!
             */
            default: return;
        }
    }

    static void _sgRF_handleGateway(sgRF_radio_t* radio){
        switch(radio->_state) {
            /**
             * STBY
             *      the radio is in standby and available for config changes
             */
            case STBY:
                break;
            /**
             * GW_BROADCAST
             *      expl
             */
            case GW_BROADCAST:
                /* count the transmitted packets */
                if (radio->_tx_flag) {
                    #ifdef DEBUG
                    debug.tx_count++;
                    #endif
                    radio->_tx_flag = false;
                }
                break;
            /**
             * GW_TRANS
             *      expl
             */
            case GW_TRANS:
                break;
            /**
             * OFF
             *      probably unintialized, do nothing!
             */
            default: return;
        }
    }


    void sgRF_handle(sgRF_radio_t* radio) {

        _sgRF_serviceIrq(radio);

        if(radio->_rt_flag) {
            // TODO handle rt interrupt (unreachable while auto retransmit is off)
            radio->_rt_flag = false;
        }

        if (radio->mode == FIXTURE) _sgRF_handleFixture(radio);
        if (radio->mode == GATEWAY) _sgRF_handleGateway(radio);
    }

