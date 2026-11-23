#include "dmx.h"
#include "cybsp.h"
#include "cy_utils.h"
#include "seeGridRF.h"
#include <string.h>

/* #define TESTING */   /* send a rotating colorwheel instead of received DMX */

#ifdef TESTING
extern volatile uint32_t msTicks;
static uint32_t timer = 0;

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

/**
 * Test pattern: rotating color wheel
 * universe is filled with fixture frames (layout, width and byte order from seeGridRF.h),
 * hue spread over all fixtures, white off
 */
#define TEST_FRAME_PERIOD_MS    40      /* ~25 frames/s */
#define TEST_CHANNELS           512
#define TEST_FIXTURES           (TEST_CHANNELS / FIXTURE_FRAME_BYTES)
#define TEST_HUE_STEP           4       /* hue advance per frame, 0..1535 per revolution */
#define TEST_CH_MAX             ((1UL << (8 * FIXTURE_CH_BYTES)) - 1)

/* 8 bit -> full fixture channel range */
static sgRF_fixCh_t scale(uint8_t v) {
    return (sgRF_fixCh_t)(v * TEST_CH_MAX / 255);
}

/* hue 0..1535 (6 sectors * 256) -> full saturation/brightness color */
static void hue_to_frame(uint16_t hue, sgRF_fixFrame_t* fix) {
    uint8_t f = hue & 0xFF;
    uint8_t r, g, b;
    switch (hue >> 8) {
        case 0:  r = 255;     g = f;       b = 0;       break;
        case 1:  r = 255 - f; g = 255;     b = 0;       break;
        case 2:  r = 0;       g = 255;     b = f;       break;
        case 3:  r = 0;       g = 255 - f; b = 255;     break;
        case 4:  r = f;       g = 0;       b = 255;     break;
        default: r = 255;     g = 0;       b = 255 - f; break;
    }
    fix->white = 0;
    fix->red   = scale(r);
    fix->green = scale(g);
    fix->blue  = scale(b);
}

static void test_colorwheel(dmx_buf_t buf) {
    static uint16_t hue = 0;
    sgRF_fixFrame_t fix;
    for (uint16_t i = 0; i < TEST_FIXTURES; i++) {
        hue_to_frame((hue + (uint32_t)i * 1536 / TEST_FIXTURES) % 1536, &fix);
        sgRF_writeFrame(&buf[i * FIXTURE_FRAME_BYTES], &fix);
    }
    hue = (hue + TEST_HUE_STEP) % 1536;
}
#endif

/**
 * Enttec DMX USB Pro protocol (host -> widget), received on the debug UART
 *      0x7E | label | len LSB | len MSB | data[len] | 0xE7
 *      label 6 (Output Only Send DMX Packet): data = start code + up to 512 channels
 */
#define ENTTEC_SOM              0x7E
#define ENTTEC_EOM              0xE7
#define ENTTEC_LABEL_SEND_DMX   6
#define ENTTEC_LABEL_SEND_RDM   7
#define ENTTEC_LABEL_RDM_DISC   11
#define ENTTEC_MAX_LEN          600

static void enttec_rx(dmx_t* dmx, const uint8_t byte) {
    switch (dmx->_rx_state) {
        case _DMX_RX_SOM:
            if (byte == ENTTEC_SOM) dmx->_rx_state = _DMX_RX_LABEL;
            break;
        case _DMX_RX_LABEL:
            dmx->_rx_label = byte;
            dmx->_rx_state = _DMX_RX_LEN_LO;
            break;
        case _DMX_RX_LEN_LO:
            dmx->_rx_len = byte;
            dmx->_rx_state = _DMX_RX_LEN_HI;
            break;
        case _DMX_RX_LEN_HI:
            dmx->_rx_len |= (uint16_t)byte << 8;
            dmx->_rx_pos = 0;
            if (dmx->_rx_len > ENTTEC_MAX_LEN) {
                dmx->errors++;
                dmx->_rx_state = _DMX_RX_SOM;
                break;
            }
            dmx->_rx_keep = (dmx->_rx_label == ENTTEC_LABEL_SEND_DMX);
            /* the host waits for each RDM reply, so a pending request means a protocol error */
            dmx->_rdm_keep = (dmx->_rx_label == ENTTEC_LABEL_SEND_RDM || dmx->_rx_label == ENTTEC_LABEL_RDM_DISC)
                             && !dmx->_rdm_ready && dmx->_rx_len <= DMX_RDM_MAX;
            dmx->_rx_state = dmx->_rx_len ? _DMX_RX_DATA : _DMX_RX_EOM;
            break;
        case _DMX_RX_DATA:
            if (dmx->_rx_keep && dmx->_rx_pos < sizeof(dmx->_rx_data[0])) {
                dmx->_rx_data[dmx->_rx_fill][dmx->_rx_pos] = byte;
            }
            if (dmx->_rdm_keep) {
                dmx->_rdm_data[dmx->_rx_pos] = byte;
            }
            if (++dmx->_rx_pos >= dmx->_rx_len) dmx->_rx_state = _DMX_RX_EOM;
            break;
        case _DMX_RX_EOM:
            if (byte != ENTTEC_EOM) {
                dmx->errors++;
            }
            else if (dmx->_rdm_keep) {
                dmx->_rdm_len = dmx->_rx_len;
                dmx->_rdm_discovery = (dmx->_rx_label == ENTTEC_LABEL_RDM_DISC);
                dmx->_rdm_ready = true;
            }
            else if (dmx->_rx_keep && dmx->_rx_data[dmx->_rx_fill][0] == 0x00) {
                /* start code 0 = dimmer data; others (RDM, text, ...) are ignored.
                   publish this buffer (newest frame wins) and fill the other one next */
                const uint16_t n = (dmx->_rx_len < sizeof(dmx->_rx_data[0])) ? dmx->_rx_len : sizeof(dmx->_rx_data[0]);
                dmx->_rx_channels[dmx->_rx_fill] = n - 1;
                dmx->_rx_fill ^= 1;
                dmx->_rx_ready = true;
                dmx->frames++;
            }
            dmx->_rx_state = _DMX_RX_SOM;
            break;
    }
}

/**
 * User Commands
 */
    #ifdef TESTING
    void dmx_init(dmx_t* dmx) {
        timer_reset();
    }
    #else
    void dmx_init(dmx_t* dmx){
        /* UART, RX FIFO and its interrupt node are configured via config tool */
        dmx->_rx_state = _DMX_RX_SOM;
        dmx->_rx_fill = 0;
        dmx->_rx_ready = false;
        NVIC_ClearPendingIRQ(dmx->irqN);
        NVIC_EnableIRQ(dmx->irqN);
    }
    #endif
    
    #ifdef TESTING
    void dmx_handle(dmx_t* dmx){
        if (timer_expired(TEST_FRAME_PERIOD_MS)) {
            test_colorwheel(dmx->buffer);
            dmx->_new_frame = true;
            timer_reset();
        }
    }
    #else
    void dmx_handle(dmx_t* dmx) {
        if (!dmx->_rx_ready) return;
        /* runs in the main loop like the broadcast, so the universe never changes mid-broadcast.
           the interrupt is held off during the copy (~50 us, the RX FIFO covers 320 us),
           so it cannot publish into the buffer being copied */
        NVIC_DisableIRQ(dmx->irqN);
        const uint8_t ready = dmx->_rx_fill ^ 1;
        const uint16_t channels = dmx->_rx_channels[ready];
        memcpy(dmx->buffer, &dmx->_rx_data[ready][1], channels);
        memset(dmx->buffer + channels, 0, DMX_CHANNELS - channels);
        dmx->_rx_ready = false;
        NVIC_EnableIRQ(dmx->irqN);
        dmx->_new_frame = true;
    }
    #endif

    bool dmx_rdmRequest(dmx_t* dmx, const uint8_t** data, uint16_t* len, bool* discovery) {
        if (!dmx->_rdm_ready) return false;
        *data = dmx->_rdm_data;
        *len = dmx->_rdm_len;
        *discovery = dmx->_rdm_discovery;
        return true;
    }

    void dmx_rdmDone(dmx_t* dmx) {
        dmx->_rdm_ready = false;
    }

    void dmx_send(dmx_t* dmx, const uint8_t label, const uint8_t* data, const uint16_t len) {
        const uint8_t head[4] = { ENTTEC_SOM, label, (uint8_t)len, (uint8_t)(len >> 8) };
        for (uint8_t i = 0; i < sizeof(head); i++) XMC_UART_CH_Transmit(dmx->usic_ch, head[i]);
        for (uint16_t i = 0; i < len; i++) XMC_UART_CH_Transmit(dmx->usic_ch, data[i]);
        XMC_UART_CH_Transmit(dmx->usic_ch, ENTTEC_EOM);
    }

    void dmx_rxIrqHandler(dmx_t* dmx) {
        while (!XMC_USIC_CH_RXFIFO_IsEmpty(dmx->usic_ch)) {
            enttec_rx(dmx, (uint8_t)XMC_USIC_CH_RXFIFO_GetData(dmx->usic_ch));
        }
    }

    bool dmx_newFrame(dmx_t* dmx) {
        return dmx->_new_frame;
    }

    void dmx_clearFrame(dmx_t* dmx) {
        dmx->_new_frame = false;
    }