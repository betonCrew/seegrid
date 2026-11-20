#ifndef _NRF24L01_H
#define _NRF24L01_H

#include "stdint.h"
#include "cybsp.h"
#include "cy_utils.h"

#define XMC_RADIO_PARAMS    XMC_USIC_CH_t *const spi, XMC_SPI_CH_SLAVE_SELECT_t const selo, XMC_GPIO_PORT_t* ce_port, uint8_t ce_pin

#ifdef XMC1
    #define NRF24_PARAMS XMC_RADIO_PARAMS
#elif LINUX
    #define NRF24_PARAMS
#endif


/**
 * Register and Bit Definitions
 */

    /* Register Map */
    #define NRF_CONFIG                  0x00        
    #define NRF_CONFIG_RES_STATE        0x08
    #define NRF_EN_AA                   0x01        
    #define NRF_EN_AA_RES_STATE         0x3F
    #define NRF_EN_RXADDR               0x02        
    #define NRF_EN_RXADDR_RES_STATE     0x03
    #define NRF_SETUP_AW                0x03        
    #define NRF_SETUP_AW_RES_STATE      0x03
    #define NRF_SETUP_RETR              0x04        
    #define NRF_SETUP_RETR_RES_STATE    0x03
    #define NRF_RF_CH                   0x05        
    #define NRF_RF_CH_RES_STATE         0x02
    #define NRF_RF_SETUP                0x06        
    #define NRF_RF_SETUP_RES_STATE      0x0F
    #define NRF_STATUS                  0x07
    #define NRF_OBSERVE_TX              0x08
    #define NRF_RPD                     0x09
    #define NRF_RX_ADDR_P0              0x0A
    #define NRF_RX_ADDR_P1              0x0B
    #define NRF_RX_ADDR_P2              0x0C
    #define NRF_RX_ADDR_P3              0x0D
    #define NRF_RX_ADDR_P4              0x0E
    #define NRF_RX_ADDR_P5              0x0F
    #define NRF_TX_ADDR                 0x10
    #define NRF_RX_PW_P0                0x11
    #define NRF_RX_PW_P1                0x12
    #define NRF_RX_PW_P2                0x13
    #define NRF_RX_PW_P3                0x14
    #define NRF_RX_PW_P4                0x15
    #define NRF_RX_PW_P5                0x16
    #define NRF_FIFO_STATUS             0x17
    #define NRF_DYNPD                   0x1C
    #define NRF_FEATURE                 0x1D

    /* 0x00 CONFIG */
    #define MASK_RX_DR_BIT      6
    #define MASK_RX_DR_MASK     (0b1 << MASK_RX_DR_BIT)
    #define MASK_TX_DS_BIT      5
    #define MASK_TX_DS_MASK      (0b1 << MASK_TX_DS_BIT)
    #define MASK_MAX_RT_BIT     4
    #define MASK_MAX_RT_MASK    (0b1 << MASK_MAX_RT_BIT)
    #define EN_CRC_BIT          3
    #define EN_CRC_MASK         (0b1 << EN_CRC_BIT)
    #define CRCO_BIT            2
    #define CRC0_MASK           (0b1 << CRCO_BIT)
    #define PWR_UP_BIT          1
    #define PWR_UP_MASK         (0b1 << PWR_UP_BIT)
    #define PRIM_RX_BIT         0
    #define PRIM_RX_MASK        (0b1 << PRIM_RX_BIT)

    /* 0x01 EN_AA */
    #define ENAA_P5_BIT         5
    #define ENAA_P5_MASK        (0b1 << ENAA_P5_BIT)
    #define ENAA_P4_BIT         4
    #define ENAA_P4_MASK        (0b1 << ENAA_P4_BIT)
    #define ENAA_P3_BIT         3
    #define ENAA_P3_MASK        (0b1 << ENAA_P3_BIT)
    #define ENAA_P2_BIT         2
    #define ENAA_P2_MASK        (0b1 << ENAA_P2_BIT)
    #define ENAA_P1_BIT         1
    #define ENAA_P1_MASK        (0b1 << ENAA_P1_BIT)
    #define ENAA_P0_BIT         0
    #define ENAA_P0_MASK        (0b1 << ENAA_P0_BIT)

    /* 0x02 EN_RXADDR */
    #define ERX_P5_BIT          5
    #define ERX_P5_MASK         (0b1 << ERX_P5_BIT)
    #define ERX_P4_BIT          4
    #define ERX_P4_MASK         (0b1 << ERX_P4_BIT)
    #define ERX_P3_BIT          3
    #define ERX_P3_MASK         (0b1 << ERX_P3_BIT)
    #define ERX_P2_BIT          2
    #define ERX_P2_MASK         (0b1 << ERX_P2_BIT)
    #define ERX_P1_BIT          1
    #define ERX_P1_MASK         (0b1 << ERX_P1_BIT)
    #define ERX_P0_BIT          0
    #define ERX_PO_MASK         (0b1 << ERX_P0_BIT)

    /* 0x03 SETUP_AW */
    #define AW_BIT              0
    #define AW_MASK             (0b11 << AW_BIT)

    /* 0x04 SETUP_RETR */
    #define ARD_BIT             4
    #define ARD_MASK            (0b1111 << ARD_BIT)
    #define ARC_BIT             0
    #define ARC_MASK            (0b1111 << ARC_BIT)

    /* 0x05 RF_CH */
    #define CH_BIT              0
    #define CH_MASK             0b01111111

    /* 0x06 RF_SETUP */
    #define RF_PWR_BIT          1
    #define RF_PWR_MASK         (0b11 << RF_PWR_BIT)
    #define RF_DR_H_BIT         3
    #define RF_DR_H_MASK        (0b1 << RF_DR_H_BIT)
    #define PLL_LOCK_BIT        4
    #define PLL_LOCK_MASK       (0b1 << PLL_LOCK_BIT)
    #define RF_DR_L_BIT         5
    #define RF_DR_L_MASK        (0b1 << RF_DR_L_BIT)
    #define CONT_WAVE_BIT       7
    #define CONT_WAVE_MASK      (0b1 << CONT_WAVE_BIT)

    /* 0x07 STATUS */
    #define RX_DR_BIT           6
    #define RX_DR_MASK          (0b1 << RX_DR_BIT)
    #define TX_DS_BIT           5
    #define TX_DS_MASK          (0b1 << TX_DS_BIT)
    #define MAX_RT_BIT          4
    #define MAX_RT_MASK         (0b1 << MAX_RT_BIT)
    #define RX_P_NO_BIT         1
    #define RX_P_NO_MASK        (0b111 << RX_P_NO_BIT)
    #define TX_FIFO_FULL_BIT    0
    #define TX_FIFO_FULL_MASK   (0b1 << TX_FIFO_FULL_BIT)

    /* 0x08 OBSERVE_TX */
    #define PLOS_CNT_BIT        4
    #define PLOS_CNT_MASK       (0b1111 << PLOS_CNT_BIT)
    #define ARC_CNT_BIT         0
    #define ARC_CNT_MASK        (0b1111 << ARC_CNT_BIT)

    /* 0x09 RPD */
    #define RPD_BIT             0
    #define RPD_MASK            (0b1 << RPD_BIT)

    /* 0x0A RX_ADDR_P0 */
    /* 0x0B RX_ADDR_P1 */
    /* 0x0C RX_ADDR_P2 */
    /* 0x0D RX_ADDR_P3 */
    /* 0x0E RX_ADDR_P4 */
    /* 0x0F RX_ADDR_P5 */

    /* 0x10 TX_ADDR */

    /* 0x11 RX_PW_P0 */
    /* 0x12 RX_PW_P1 */
    /* 0x13 RX_PW_P2 */
    /* 0x14 RX_PW_P3 */
    /* 0x15 RX_PW_P4 */
    /* 0x16 RX_PW_P5 */
    #define RX_PW_PX_MASK       0x00111111

    /* 0x17 FIFO_STATUS */
    #define TX_REUSE_BIT        6
    #define TX_REUSE_MASK       (0b1 << TX_REUSE_BIT)
    #define TX_FULL_BIT         5
    #define TX_FULL_MASK        (0b1 << TX_FULL_BIT)
    #define TX_EMPTY_BIT        4
    #define TX_EMPTY_MASK       (0b1 << TX_EMPTY_BIT)
    #define RX_FULL_BIT         1
    #define RX_FULL_MASK        (0b1 << RX_FULL_BIT)
    #define RX_EMPTY_BIT        0
    #define RX_EMPTY_MASK       (0b1 << RX_EMPTY_BIT)

    /* 0x1C DYNPD */
    #define DPL_P5_BIT          5
    #define DPL_P5_MASK         (0b1 << DPL_P5_BIT)
    #define DPL_P4_BIT          4
    #define DPL_P4_MASK         (0b1 << DPL_P4_BIT)
    #define DPL_P3_BIT          3
    #define DPL_P3_MASK         (0b1 << DPL_P3_BIT)
    #define DPL_P2_BIT          2
    #define DPL_P2_MASK         (0b1 << DPL_P2_BIT)
    #define DPL_P1_BIT          1
    #define DPL_P1_MASK         (0b1 << DPL_P1_BIT)
    #define DPL_P0_BIT          0
    #define DPL_P0_MASK         (0b1 << DPL_P0_BIT)

    /* 0x1D FEATURE */
    #define EN_DPL_BIT          2
    #define EN_DPL_MASK         (0b1 << EN_DPL_BIT)
    #define EN_ACK_PAY_BIT      1
    #define EN_ACK_PAY_MASK     (0b1 << EN_ACK_PAY_BIT)
    #define EN_DYN_ACK_BIT      0
    #define EN_DYN_ACK_MASK     (0b1 << EN_DYN_ACK_BIT)

    /* SPI COMMANDS */
    #define R_REGISTER          0x00
    #define W_REGISTER          0x20
        #define REGISTER_MASK       0x1F

    #define R_RX_PAYLOAD        0x61
    #define W_TX_PAYLOAD        0xA0
    
    #define FLUSH_TX            0xE1
    #define FLUSH_RX            0xE2
    #define REUSE_TX_PL         0xE3
    
    #define R_RX_PL_WID         0x60
    #define W_ACK_PAYLOAD       0xA8
        #define ACK_PAYLOAD_MASK    0x07
    #define W_TX_PAYLOAD_NO_ACK 0xB0

    #define NRF_NOP            0xFF

/**
 * SPI Bus Definitions
 */

    #define NRF_WLEN        8


/**
 * SPI-Commands
 * 
 *      Every new command must be started by a high to low transition on CSN.
 *      The STATUS register is serially shifted out on the MISO pin simultaneously to
 *      the SPI command word shifting to the MOSI pin.
 *      
 *      The serial shifting SPI commands is in the following format:
 *          <Command word: MSBit to LSBit (one byte)>
 *          <Data bytes: LSByte to MSByte, MSBit in each byte first>
 * 
 *      The W_REGISTER and R_REGISTER commands operate on single or multi-byte registers. 
 *      When accessing multi-byte registers read or write to the MSBit of LSByte first.
 *      You can terminate the writing before all bytes in a multi-byte register are written,
 *      leaving the unwritten MSByte(s) unchanged. For example, the LSByte of RX_ADDR_P0 can
 *      be modified by writing only one byte to the RX_ADDR_P0 register. The content of the
 *      status register is always read to MISO after a high to low transition on CSN.
 */

    /**
     *  Read command and status registers.
     *      addr - 5 bit Register Map Address
     */
    void _nrf24_readReg(XMC_USIC_CH_t *const spi, XMC_SPI_CH_SLAVE_SELECT_t const selo, const uint8_t addr, char* data, const uint8_t len);

    /**
     *  Write command and status registers.
     *      addr - 5 bit Register Map Address
     * 
     *  Executable in power down or standby modesonly.
     */
    void _nrf24_writeReg(XMC_USIC_CH_t *const spi, XMC_SPI_CH_SLAVE_SELECT_t const selo, const uint8_t addr, const char* data, const uint8_t len);

    /**
     *  Write a packet wird len words into TX-Fifo
     */
    void _nrf24_writeTx(XMC_USIC_CH_t *const spi, XMC_SPI_CH_SLAVE_SELECT_t const selo, const char* data, const uint8_t len);

    /**
     *  Read len words from RX-Fifo
     */
    void _nrf24_readRx(XMC_USIC_CH_t *const spi, XMC_SPI_CH_SLAVE_SELECT_t const selo, char* data, const uint8_t len);



    /**
     *  Used for a PTX device Reuse last transmitted payload.
     *  TX payload reuse is active until W_TX_PAYLOAD or FLUSH TX is executed.
     *  TX payload reuse must not be activated or deactivated during package transmission.
     */
    void _nrf24_reuseTx(void);

    /**
     *  Read RX payload width for the top R_RX_PAYLOAD in the RX FIFO.
     *  Note: Flush RX FIFO if the read value is larger than 32 bytes.
     */
    void _nrf24_readRxWidth(uint8_t* width);

    /**
     *  Used in RX mode. Write Payload to be transmitted together with ACK packet on PIPE PPP.
     *  (PPP valid in the range from 000 to 101).
     * 
     *  Maximum three ACK packet payloads can be pending.
     *  Payloads with same PPP are handled using first in - first out principle.
     *  Write payload: 1–32 bytes. A write operation always starts at byte 0.
     */
    void _nrf24_writeAckPl(const char* payload);

    /**
     *  Used in TX mode. Disables AUTOACK on this specific packet.
     */
    void _nrf24_writeTxNoAck(const char* payload);

    /**
     *  No Operation. Might be used to read the STATUS register
     */
    void _nrf24_nop(NRF24_PARAMS);

#endif