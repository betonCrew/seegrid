
#include "cybsp.h"
#include "cy_utils.h"

#include "_nrf24l01.h"
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
 * Initialization
 */

    void nrf24_init(NRF24_PARAMS, XMC_USIC_t *const usic) {
        /* most is already configured via mtb config tool */
        XMC_USIC_Enable(usic);
        XMC_GPIO_SetOutputLow(ce_port, ce_pin);
    }

/**
 * Reset Device
 */

    void nrf24_reset(NRF24_PARAMS) {
        /* set every register to its reset state is supposed to reset device */
        
    }

/**
 * Device Power
 */

    void nrf24_powerDown(NRF24_PARAMS) {
        char config;
        _nrf24_readReg(spi, NRF_CONFIG, &config, 1);
        config &= ~PWR_UP_MASK;
        _nrf24_writeReg(spi, NRF_CONFIG, &config, 1);
    }

    void nrf24_powerUp(NRF24_PARAMS) {
        char config;
        _nrf24_readReg(spi, NRF_CONFIG, &config, 1);
        config |= PWR_UP_MASK;
        _nrf24_writeReg(spi, NRF_CONFIG, &config, 1);
    }

/**
 * Primary Transceiver Direction
 */

    void nrf24_setPrimTx(NRF24_PARAMS) {
        char config = 0;
        _nrf24_readReg(spi, NRF_CONFIG, &config, 1);
        config &= ~PRIM_RX_MASK;
        _nrf24_writeReg(spi, NRF_CONFIG, &config, 1);
    }

    void nrf24_setPrimRx(NRF24_PARAMS) {
        char config = 0;
        _nrf24_readReg(spi, NRF_CONFIG, &config, 1);
        config |= PRIM_RX_MASK;
        _nrf24_writeReg(spi, NRF_CONFIG, &config, 1);
    }




/**
 * The RF channel frequency determines the center of the channel used by the nRF24L01+. The channel
 * occupies a bandwidth of less than 1MHz at 250kbps and 1Mbps and a bandwidth of less than 2MHz at
 * 2Mbps. nRF24L01+ can operate on frequencies from 2.400GHz to 2.525GHz. The programming resolu-
 * tion of the RF channel frequency setting is 1MHz.
 * At 2Mbps the channel occupies a bandwidth wider than the resolution of the RF channel frequency setting.
 * To ensure non-overlapping channels in 2Mbps mode, the channel spacing must be 2MHz or more. At
 * 1Mbps and 250kbps the channel bandwidth is the same or lower than the resolution of the RF frequency.
 * The RF channel frequency is set by the RF_CH register according to the following formula:
 * F0= 2400 + RF_CH [MHz]
 */

    void nrf24_getChannel(NRF24_PARAMS, uint8_t* channel) {
        char rf_ch = 0;
        _nrf24_readReg(spi, NRF_RF_CH, &rf_ch, 1);
        *channel = rf_ch;
    }

    void nrf24_setChannel(NRF24_PARAMS, const uint8_t channel) {
        char rf_ch = channel & 0x01111111;
        _nrf24_writeReg(spi, NRF_RF_CH, &rf_ch, 1);
    }

/**
 * Power Amplifier
 */

    void nrf24_setPowerLevel(NRF24_PARAMS, const nrf24_rfPwr_t level) {
        char rf_setup = 0;
        _nrf24_readReg(spi, NRF_RF_SETUP, &rf_setup, 1);
        rf_setup &= ~RF_PWR_MASK;
        rf_setup |= (level << RF_PWR_BIT);
        _nrf24_writeReg(spi, NRF_RF_SETUP, &rf_setup, 1);
    }

    const nrf24_rfPwr_t nrf24_getPowerLevel(NRF24_PARAMS) {
        char rf_setup = 0;
        _nrf24_readReg(spi, NRF_RF_SETUP, &rf_setup, 1);
        return((nrf24_rfPwr_t)((rf_setup & RF_PWR_MASK) >> RF_PWR_BIT));
    }

/**
 * Data Rate
 */

    void nrf24_setDataRate(NRF24_PARAMS, const nrf24_dataRate_t rate) {
        char rf_setup = 0;
        _nrf24_readReg(spi, NRF_RF_SETUP, &rf_setup, 1);
        rf_setup &= ~(RF_DR_H_MASK | RF_DR_L_MASK);
        /* L_Bit (5) is bit 1 in rate */
        rf_setup |= RF_DR_L_MASK & (rate << (RF_DR_L_BIT-1));
        /* H_Bit (3) is bit 0 in rate */
        rf_setup |= RF_DR_H_MASK & (rate << RF_DR_H_BIT);
        _nrf24_writeReg(spi, NRF_RF_SETUP, &rf_setup, 1);
    }


/**
 * Address Width
 */

    void nrf24_setAddrWidth(NRF24_PARAMS, const nrf24_aW_t addrWidth) {
        const char width = addrWidth & AW_MASK;
        _nrf24_writeReg(spi, NRF_SETUP_AW, &width, 1);
    }

    nrf24_aW_t nrf24_getAddrWidth(NRF24_PARAMS) {
        char width = 0;
        _nrf24_readReg(spi, NRF_SETUP_AW, &width, 1);
        width &= AW_MASK;
        return width;
    }

/**
 * CRC
 */

    void nrf24_setCrc(NRF24_PARAMS, const nrf24_crc_t scheme) {
        char config = 0;
        _nrf24_readReg(spi, NRF_CONFIG, &config, 1);
        config &= ~(EN_CRC_MASK | CRC0_MASK);
        config |= scheme << CRCO_BIT;
        _nrf24_writeReg(spi, NRF_CONFIG, &config, 1);
    }

    const nrf24_crc_t nrf24_getCrc(NRF24_PARAMS) {
        char config = 0;
        _nrf24_readReg(spi, NRF_CONFIG, &config, 1);
        config &= (CRC0_MASK | EN_CRC_MASK);
        config >>= CRCO_BIT;
        return config;
    }

/**
 * IRQ Mask
 */

    void nrf24_setupFlags(NRF24_PARAMS, const uint8_t flags) {
        char config = 0;
        _nrf24_readReg(spi, NRF_CONFIG, &config, 1);
        config &= ~(MASK_RX_DR_MASK | MASK_MAX_RT_MASK | MASK_TX_DS_MASK);
        config |= flags & (MASK_RX_DR_MASK | MASK_TX_DS_MASK | MASK_MAX_RT_MASK);
        _nrf24_writeReg(spi, NRF_CONFIG, &config, 1);
    }

/**
 * Status Flags
 */

    void nrf24_clearFlags(NRF24_PARAMS, const uint8_t flags) {
        const char status = flags & (RX_DR_MASK | TX_DS_MASK | MAX_RT_MASK);
        _nrf24_writeReg(spi, NRF_STATUS, &status, 1);
    }

    const uint8_t nrf24_readStatus(NRF24_PARAMS) {
        
        const uint8_t cmd = NRF_NOP;
        uint8_t status;
        XMC_USIC_CH_SetFrameLength(spi, 1*NRF_WLEN);
        XMC_USIC_CH_TXFIFO_PutData(spi, cmd);
        while(XMC_USIC_CH_RXFIFO_IsEmpty(spi))
        {}
        status = XMC_USIC_CH_RXFIFO_GetData(spi);
        #ifdef DEBUG
        debug.nrf24_status = status;
        #endif      
        return status;
    }

    const uint8_t nrf24_getStatus(NRF24_PARAMS) {
        // where are the last received flags stored?
        return 0;
    }

/**
 * Auto Acknowledgement
 */

    void nrf24_disableAutoAck(NRF24_PARAMS, const uint8_t ch_mask) {
        char pipes = 0;
        _nrf24_readReg(spi, NRF_EN_AA, &pipes, 1);
        pipes &= ~ch_mask;
        _nrf24_writeReg(spi, NRF_EN_AA, &pipes, 1);
    }

    void nrf24_enableAutoAck(NRF24_PARAMS, const uint8_t ch_mask) {
        char pipes = 0;
        _nrf24_readReg(spi, NRF_EN_AA, &pipes, 1);
        pipes |= ch_mask;
        _nrf24_writeReg(spi, NRF_EN_AA, &pipes, 1);
    }

/**
 * Auto Retransmit
 */

    void nrf24_disableAutoRetransmit(NRF24_PARAMS) {
        char retrans = 0;
        _nrf24_writeReg(spi, NRF_SETUP_RETR, &retrans, 1);
    }

/**
 * Rx Address / Pipe Enable
 */

    void nrf24_enableRxAddr(NRF24_PARAMS, uint8_t pipe_mask) {
        char reg = 0;
        _nrf24_readReg(spi, NRF_EN_RXADDR, &reg, 1);
        reg |= pipe_mask & 0x3F;
        _nrf24_writeReg(spi, NRF_EN_RXADDR, &reg, 1);
    }

    void nrf24_disableRxAddr(NRF24_PARAMS, uint8_t pipe_mask) {
        char reg = 0;
        _nrf24_readReg(spi, NRF_EN_RXADDR, &reg, 1);
        reg &= ~(pipe_mask & 0x3F);
        _nrf24_writeReg(spi, NRF_EN_RXADDR, &reg, 1);
    }

/**
 * Transmit Address
 */

    void nrf24_setTxAddr(NRF24_PARAMS, const uint64_t addr) {
        _nrf24_writeReg(spi, NRF_TX_ADDR, (char*)&addr, 5);
    }

    const uint64_t nrf24_getTxAddr(NRF24_PARAMS) {
        uint64_t addr = 0;
        _nrf24_readReg(spi, NRF_TX_ADDR, (char*)&addr, 5);
        return addr;
    }

/**
 * Receive Address
 */

    void nrf24_setRxAddr(NRF24_PARAMS, const uint8_t pipe, const uint64_t addr) {
        if (pipe >= 6) return;
        /* there is a rx addr register for each pipe starting at 0x0A*/
        _nrf24_writeReg(spi, NRF_RX_ADDR_P0+pipe, (char*)&addr, 5);
    }

    const uint64_t nrf24_getRxAddr(NRF24_PARAMS, const uint8_t pipe) {
        uint64_t addr = 0;
        _nrf24_readReg(spi, NRF_RX_ADDR_P0+pipe, (char*)&addr, 5);
        return addr;
    }

/**
 * Payload Width
 */

    void nrf24_setPayloadWidth(NRF24_PARAMS, const uint8_t pipe, const uint8_t width) {
        if (pipe >= 6) return;
        _nrf24_writeReg(spi, NRF_RX_PW_P0+pipe, (char*)&width, 1);
    }

    const uint8_t nrf24_getPayloadWidth(NRF24_PARAMS, const uint8_t pipe) {
        if (pipe >= 6) return 0x00;
        char width;
        _nrf24_readReg(spi, NRF_RX_PW_P0+pipe, (char*)&width, 1);
        return width;        
    }

/**
 * Fifo Status
 */

    const bool nrf24_txFifoEmpty(NRF24_PARAMS) {
        char status;
        _nrf24_readReg(spi, NRF_FIFO_STATUS, &status, 1);
        return (status & TX_EMPTY_MASK);
    }

    const bool nrf24_txFifoFull(NRF24_PARAMS) {
        const char status = nrf24_readStatus(spi, ce_port, ce_pin);
        return (status & TX_FIFO_FULL_MASK);
    }
 
    const bool nrf24_rxFifoEmpty(NRF24_PARAMS) {
        char status;
        _nrf24_readReg(spi, NRF_FIFO_STATUS, &status, 1);
        return (status & RX_EMPTY_MASK);
    }

    const bool nrf24_rxFifoFull(NRF24_PARAMS) {
        char status;
        _nrf24_readReg(spi, NRF_FIFO_STATUS, &status, 1);
        return (status & RX_FULL_MASK);
    }

/**
 * Dynamic Payload
 */

    void nrf24_disableDynPayload(NRF24_PARAMS, const uint8_t ch_mask) {
        char pipes = 0;
        _nrf24_readReg(spi, NRF_DYNPD, &pipes, 1);
        pipes &= ~ch_mask;
        _nrf24_writeReg(spi, NRF_DYNPD, &pipes, 1);
    }

    void nrf24_enableDynPayload(NRF24_PARAMS, const uint8_t ch_mask) {
        char pipes = 0;
        _nrf24_readReg(spi, NRF_DYNPD, &pipes, 1);
        pipes |= ch_mask;
        _nrf24_writeReg(spi, NRF_DYNPD, &pipes, 1);
    }

/**
 * Fifo Access
 */

    /**
     *  The RX_DR IRQ is asserted by a new packet arrival event.
     *  The procedure for handling this interrupt should be:
     *      1) read payload through SPI,
     *      2) clear RX_DR IRQ,
     *      3) read FIFO_STATUS to check if there are more payloads available in RX FIFO,
     *      4) if there are more data in RX FIFO, repeat from step 1).
     */
    void nrf24_readRx(NRF24_PARAMS, char* data, const uint8_t payloads, const uint8_t width) {
        uint8_t i = 0;
        while(i < payloads)
            if(!nrf24_rxFifoEmpty(spi, ce_port, ce_pin)) {
                _nrf24_readRx(spi, (data+(i*width)), width);
                nrf24_clearFlags(spi, ce_port, ce_pin, RX_DR_MASK);
                i++;
            }
            // FIXME some kind of timeout?
    }

    void nrf24_writeTx(NRF24_PARAMS, const char* data, const uint8_t width) {
        bool succ = false;
        while (!succ) {
            if(!nrf24_txFifoFull(spi, ce_port, ce_pin)) {
                _nrf24_writeTx(spi, data, width);
                succ = true;
            }
            // FIXME some kind of timeout?
        }
    }

/**
 * Fifo Flush
 */

    void nrf24_flushTx(NRF24_PARAMS) {
        const uint8_t cmd = FLUSH_TX;
        uint8_t status;

        XMC_USIC_CH_TXFIFO_Flush(spi);
        XMC_USIC_CH_RXFIFO_Flush(spi);

        XMC_USIC_CH_SetFrameLength(spi, 1*NRF_WLEN);
        XMC_USIC_CH_TXFIFO_PutData(spi, cmd);
        while(XMC_USIC_CH_RXFIFO_IsEmpty(spi))
        {}
        status = XMC_USIC_CH_RXFIFO_GetData(spi);
        #ifdef DEBUG
        debug.nrf24_status = status;
        #endif  
        XMC_DelayUs(5);
    }

    void nrf24_flushRx(NRF24_PARAMS) {
        const uint8_t cmd = FLUSH_RX;
        uint8_t status;

        XMC_USIC_CH_TXFIFO_Flush(spi);
        XMC_USIC_CH_RXFIFO_Flush(spi);

        XMC_USIC_CH_SetFrameLength(spi, 1*NRF_WLEN);
        XMC_USIC_CH_TXFIFO_PutData(spi, cmd);
        while (XMC_USIC_CH_RXFIFO_IsEmpty(spi))
        {}
        status = XMC_USIC_CH_RXFIFO_GetData(spi);
        #ifdef DEBUG
        debug.nrf24_status = status;
        #endif  
        XMC_DelayUs(5);
    }


/**
 * Internals
 */

    /**
     * Register Read/Write
     */

        void _nrf24_readReg(XMC_USIC_CH_t *const spi, const uint8_t addr, char* data, const uint8_t len) {
            const uint8_t cmd = R_REGISTER | (REGISTER_MASK & addr);
            uint8_t bytes = 0;
            // TODO do something with this fresh status
            uint8_t status;

            XMC_USIC_CH_TXFIFO_Flush(spi);
            XMC_USIC_CH_RXFIFO_Flush(spi);
            XMC_USIC_CH_SetFrameLength(spi, (1+len)*NRF_WLEN);
            XMC_USIC_CH_TXFIFO_PutData(spi, cmd);
            for (int i = 0; i < len; i++){
                XMC_USIC_CH_TXFIFO_PutData(spi, 0x00);
            }
            while(XMC_USIC_CH_RXFIFO_IsEmpty(spi)) 
            {}
            status = (uint8_t) XMC_USIC_CH_RXFIFO_GetData(spi);
            while (bytes < len) {
                    while(XMC_USIC_CH_RXFIFO_IsEmpty(spi))
                    {}
                    *(data+bytes++) = (char) XMC_USIC_CH_RXFIFO_GetData(spi);
            }
            #ifdef DEBUG
            debug.nrf24_status = status;
            #endif  
            XMC_DelayUs(5);
        }

        void _nrf24_writeReg(XMC_USIC_CH_t *const spi, const uint8_t addr, const char* data, const uint8_t len) {
            const uint8_t cmd = W_REGISTER | (REGISTER_MASK & addr);
            uint8_t bytes = len;
            uint8_t status;
            XMC_USIC_CH_TXFIFO_Flush(spi);
            XMC_USIC_CH_RXFIFO_Flush(spi);
            XMC_USIC_CH_SetFrameLength(spi, (1+len)*NRF_WLEN);
            XMC_USIC_CH_TXFIFO_PutData(spi, cmd);
            while(bytes) {
                XMC_USIC_CH_TXFIFO_PutData(spi, *(data+(len-(bytes--))));
            }
            while(XMC_USIC_CH_RXFIFO_IsEmpty(spi))
            {}
            status = (uint8_t) XMC_USIC_CH_RXFIFO_GetData(spi);
            while(!XMC_USIC_CH_TXFIFO_IsEmpty(spi))
            {}
            #ifdef DEBUG
            debug.nrf24_status = status;
            #endif  
            XMC_DelayUs(5);
        }

    /**
     * FIFO Read/Write
     */

        void _nrf24_writeTx(XMC_USIC_CH_t *const spi, const char* data, const uint8_t len) {
            const uint8_t cmd = W_TX_PAYLOAD;
            uint8_t bytes = len;
            uint8_t status;
            XMC_USIC_CH_TXFIFO_Flush(spi);
            XMC_USIC_CH_RXFIFO_Flush(spi);
            /* control framelength by hand! */
            XMC_USIC_CH_SetFrameLength(spi, 64);
            XMC_USIC_CH_TXFIFO_PutData(spi, cmd);
            while (bytes) {
                XMC_USIC_CH_TXFIFO_PutData(spi, *(data+(len-(bytes--))));
            }
            while(XMC_USIC_CH_RXFIFO_IsEmpty(spi))
            {}
            status = (uint8_t) XMC_USIC_CH_RXFIFO_GetData(spi);
            while(!XMC_USIC_CH_TXFIFO_IsEmpty(spi))
            {}
            XMC_DelayUs(10);
            XMC_SPI_CH_ClearStatusFlag(spi, XMC_SPI_CH_STATUS_FLAG_MSLS);
            #ifdef DEBUG
            debug.nrf24_status = status;
            #endif  
            XMC_DelayUs(5);
            
        }

        void _nrf24_readRx(XMC_USIC_CH_t *const spi, char* data, const uint8_t len) {
            const uint8_t cmd = R_RX_PAYLOAD;
            uint8_t bytes = 0;
            uint8_t status;
            XMC_USIC_CH_TXFIFO_Flush(spi);
            XMC_USIC_CH_RXFIFO_Flush(spi);
            XMC_USIC_CH_SetFrameLength(spi, (1+len)*NRF_WLEN);
            XMC_USIC_CH_TXFIFO_PutData(spi, cmd);
            while(XMC_USIC_CH_RXFIFO_IsEmpty(spi)) 
            {}
            status = (uint8_t) XMC_USIC_CH_RXFIFO_GetData(spi);
            for (int i = 0; i < len; i++){
                XMC_USIC_CH_TXFIFO_PutData(spi, 0x00);
            }
            while (bytes < len) {
                    while(XMC_USIC_CH_RXFIFO_IsEmpty(spi))
                    {}
                    *(data+bytes++) = (char) XMC_USIC_CH_RXFIFO_GetData(spi);
            }
            #ifdef DEBUG
            debug.nrf24_status = status;
            #endif  
            XMC_DelayUs(5);
        }