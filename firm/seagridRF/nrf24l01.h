#ifndef NRF24L01_H
#define NRF24L01_H

/*
    Copyright (c) 2007 Stefan Engelke <mbox@stefanengelke.de>
    Portions Copyright (C) 2011 Greg Copeland

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use, copy,
    modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

/**
 * Includes
 */

    #include <stdint.h>
    #include <stdbool.h>
    #include "_nrf24l01.h"
    #include "xmc_usic.h"
    #include "xmc_gpio.h"


/**
 * Type Definitions
 */

    typedef enum {ERR, PWRDWN, STBY1, STBY2, TX, RX} radioState_t;
    typedef enum {CRC_OFF=0, CRC_1B=2, CRC_2B=3} nrf24_crc_t;
        // OFF 0b00 -> 0b0x
        // 1B  0b01 -> 0b10
        // 2B  0b10 -> 0b11
    typedef enum {P_DOWN, P_UP} nrf24_pwr_t;
    typedef enum {D_TX, D_RX} nrf24_dir_t;
    typedef enum {W_3BYTES=1, W_4BYTES, W_5BYTES} nrf24_aW_t;
    typedef enum {R_1Mbps, R_2Mbps, R_250kbps} nrf24_dataRate_t;
    typedef enum {P_18dBm, P_12dBm, P_6dBm, P_0dBM} nrf24_rfPwr_t;



/**
 * User Commands
 */

    void nrf24_init(NRF24_PARAMS, XMC_USIC_t *const usic);
    void nrf24_reset(NRF24_PARAMS);

    void nrf24_powerDown(NRF24_PARAMS);
    void nrf24_powerUp(NRF24_PARAMS);
    void nrf24_setPrimTx(NRF24_PARAMS);
    void nrf24_setPrimRx(NRF24_PARAMS);

    /**
     * Clears given flags
     *      use: RX_DR_MASK, TX_DS_MASK, MAX_RT_MASK
     */
    void nrf24_clearFlags(NRF24_PARAMS, const uint8_t flags);

    /**
     * Set which flags shall be reflected on the IRQ pin, all others are masked
     *      use: RX_DR_MASK, TX_DS_MASK, MAX_RT_MASK
     */
    void nrf24_setupFlags(NRF24_PARAMS, const uint8_t flags);

    /**
     * Get latest status byte from last SPI transaction
     *      use: RX_DR_MASK, TX_DS_MASK, MAX_RT_MASK
     */
    const uint8_t nrf24_getStatus(NRF24_PARAMS);

    /**
     * execute NOP command, to get latest status byte
     *      use: RX_DR_MASK, TX_DS_MASK, MAX_RT_MASK
     */
    const uint8_t nrf24_readStatus(NRF24_PARAMS);


    void nrf24_setDataRate(NRF24_PARAMS, const nrf24_dataRate_t rate);
    const nrf24_dataRate_t nrf24_getDataRate(NRF24_PARAMS);

    void nrf24_setCrc(NRF24_PARAMS, const nrf24_crc_t scheme);
    const nrf24_crc_t nrf24_getCrc(NRF24_PARAMS);

    void nrf24_setChannel(NRF24_PARAMS, const uint8_t channel);
    void nrf24_getChannel(NRF24_PARAMS, uint8_t* channel);

    void nrf24_setPowerLevel(NRF24_PARAMS, const nrf24_rfPwr_t level);
    const nrf24_rfPwr_t nrf24_getPowerLevel(NRF24_PARAMS);

    void nrf24_setAddrWidth(NRF24_PARAMS, const nrf24_aW_t w);
    nrf24_aW_t nrf24_getAddrWidth(NRF24_PARAMS);

    void nrf24_disableAutoAck(NRF24_PARAMS, const uint8_t ch_mask);
    void nrf24_enableAutoAck(NRF24_PARAMS, const uint8_t ch_mask);

    void nrf24_disableAutoRetransmit(NRF24_PARAMS);

    void nrf24_enableRxAddr(NRF24_PARAMS, uint8_t pipe_mask);
    void nrf24_disableRxAddr(NRF24_PARAMS, uint8_t pipe_mask);

    void nrf24_setTxAddr(NRF24_PARAMS, const uint64_t addr);
    const uint64_t nrf24_getTxAddr(NRF24_PARAMS);

    void nrf24_setRxAddr(NRF24_PARAMS, const uint8_t pipe, const uint64_t addr);
    const uint64_t nrf24_getRxAddr(NRF24_PARAMS, const uint8_t pipe);

    void nrf24_setPayloadWidth(NRF24_PARAMS, const uint8_t pipe, const uint8_t width);
    const uint8_t nrf24_getPayloadWidth(NRF24_PARAMS, const uint8_t pipe);

    void nrf24_disableDynPayload(NRF24_PARAMS, const uint8_t ch_mask);
    /**
     * requires EN_DPL and ENAA_Px
     */
    void nrf24_enableDynPayload(NRF24_PARAMS, const uint8_t ch_mask);


    /**
     *  Read RX-payload: 1 – 32 bytes. A read operation always starts at byte 0.
     *  Payload is deleted from FIFO after it is read. Used in RX mode.
     * 
     *  data represents a pointer to start of memory with size of payloads*width=words
     */
    void nrf24_readRx(NRF24_PARAMS, char* data, const uint8_t payloads, const uint8_t width);

    /**
     *  Write TX-payload: 1 – 32 bytes.
     *  A write operation always starts at byte 0 used in TX payload.
     */
    void nrf24_writeTx(NRF24_PARAMS, const char* data, const uint8_t width);


    /**
     *  true, once every payload has left the TX FIFO
     */
    const bool nrf24_txFifoEmpty(NRF24_PARAMS);

    /**
     *  true, if no payload is waiting in the RX FIFO
     */
    const bool nrf24_rxFifoEmpty(NRF24_PARAMS);

    /**
     *  Flush TX FIFO, used in TX mode
     */
    void nrf24_flushTx(NRF24_PARAMS);

    /**
     *  Flush RX FIFO, used in RX mode Should not be executed during transmission of
     *  acknowledge, that is, acknowledge package will not be completed.
     */
    void nrf24_flushRx(NRF24_PARAMS);


#endif