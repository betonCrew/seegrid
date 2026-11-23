#include "rdm.h"
#include "fixtures.h"
#include <string.h>

/* packet layout (E1.20 6.2) */
#define RDM_SC                  0xCC
#define RDM_SUB_SC              0x01
#define RDM_HEADER_LEN          24      /* start code .. PDL */
#define RDM_PD_MAX              231

#define O_MSG_LEN   2
#define O_DEST      3
#define O_SRC       9
#define O_TN        15
#define O_PORT      16      /* port id (request) / response type (response) */
#define O_SUBDEV    18
#define O_CC        20
#define O_PID       21
#define O_PDL       23
#define O_PD        24

/* command classes */
#define CC_DISC                 0x10
#define CC_GET                  0x20
#define CC_SET                  0x30

/* response types */
#define RT_ACK                  0x00
#define RT_NACK                 0x02

/* NACK reasons */
#define NR_UNKNOWN_PID          0x0000
#define NR_FORMAT_ERROR         0x0001
#define NR_UNSUPPORTED_CC       0x0005
#define NR_DATA_OUT_OF_RANGE    0x0006
#define NR_SUB_DEVICE_RANGE     0x0009

/* parameter ids */
#define PID_DISC_UNIQUE_BRANCH  0x0001
#define PID_DISC_MUTE           0x0002
#define PID_DISC_UN_MUTE        0x0003
#define PID_SUPPORTED_PARAMS    0x0050
#define PID_DEVICE_INFO         0x0060
#define PID_DEVICE_MODEL_DESC   0x0080
#define PID_MANUFACTURER_LABEL  0x0081
#define PID_SOFTWARE_VERSION    0x00C0
#define PID_DMX_PERSONALITY     0x00E0
#define PID_DMX_PERSONALITY_DESC 0x00E1
#define PID_DMX_START_ADDRESS   0x00F0
#define PID_SENSOR_DEFINITION   0x0200
#define PID_SENSOR_VALUE        0x0201
#define PID_IDENTIFY_DEVICE     0x1000

/* sensors */
#define SENSOR_BATTERY          0
#define SENSOR_DMX_RATE         1
#define SENSOR_COUNT            2
#define SENS_VOLTAGE            0x01
#define SENS_OTHER              0x7F
#define UNITS_NONE              0x00
#define UNITS_VOLTS_DC          0x02
#define PREFIX_NONE             0x00
#define PREFIX_MILLI            0x03

#define PRODUCT_CATEGORY_FIXTURE 0x0100

typedef uint64_t uid_t;
#define UID_BROADCAST_ALL       0xFFFFFFFFFFFFULL
#define UID_BROADCAST_MANUF     (((uid_t)RDM_MANUFACTURER_ID << 32) | 0xFFFFFFFFULL)

static uid_t get_uid(const uint8_t* p) {
    uid_t u = 0;
    for (uint8_t i = 0; i < 6; i++) u = (u << 8) | p[i];
    return u;
}

static void put_uid(uint8_t* p, const uid_t u) {
    for (uint8_t i = 0; i < 6; i++) p[i] = (uint8_t)(u >> (8 * (5 - i)));
}

static uid_t fixture_uid(const fixture_t* fix) {
    return ((uid_t)RDM_MANUFACTURER_ID << 32) | fix->hb.id;
}

static uint8_t* put16(uint8_t* p, const uint16_t v) { p[0] = v >> 8; p[1] = (uint8_t)v; return p + 2; }
static uint8_t* put32(uint8_t* p, const uint32_t v) { p = put16(p, v >> 16); return put16(p, (uint16_t)v); }
static uint8_t* putstr(uint8_t* p, const char* s) { const size_t n = strlen(s); memcpy(p, s, n); return p + n; }

static uint16_t checksum(const uint8_t* p, const uint16_t len) {
    uint16_t sum = 0;
    for (uint16_t i = 0; i < len; i++) sum += p[i];
    return sum;
}

/* reply buffer: Enttec receive status byte + RDM packet */
static uint8_t reply[1 + RDM_HEADER_LEN + RDM_PD_MAX + 2];

static void send_timeout(dmx_t* port) {
    dmx_send(port, DMX_LABEL_RDM_TIMEOUT, NULL, 0);
}

static void send_response(dmx_t* port, const uint8_t* req, const fixture_t* fix,
                          const uint8_t type, const uint8_t* pd, const uint8_t pdl) {
    uint8_t* r = &reply[1];
    reply[0] = 0;                               /* receive status: ok */
    r[0] = RDM_SC;
    r[1] = RDM_SUB_SC;
    r[O_MSG_LEN] = RDM_HEADER_LEN + pdl;
    memcpy(&r[O_DEST], &req[O_SRC], 6);
    put_uid(&r[O_SRC], fixture_uid(fix));
    r[O_TN] = req[O_TN];
    r[O_PORT] = type;
    r[17] = 0;                                  /* message count */
    r[O_SUBDEV] = 0;
    r[O_SUBDEV + 1] = 0;
    r[O_CC] = req[O_CC] + 1;                    /* ..._RESPONSE */
    r[O_PID] = req[O_PID];
    r[O_PID + 1] = req[O_PID + 1];
    r[O_PDL] = pdl;
    memcpy(&r[O_PD], pd, pdl);
    put16(&r[RDM_HEADER_LEN + pdl], checksum(r, RDM_HEADER_LEN + pdl));
    dmx_send(port, DMX_LABEL_RECEIVED, reply, 1 + RDM_HEADER_LEN + pdl + 2);
}

static void send_nack(dmx_t* port, const uint8_t* req, const fixture_t* fix, const uint16_t reason) {
    uint8_t pd[2];
    put16(pd, reason);
    send_response(port, req, fix, RT_NACK, pd, 2);
}

/**
 * DISC_UNIQUE_BRANCH response (E1.20 7.5): 7 x 0xFE, 0xAA, then UID and checksum
 * with every byte sent as (b | 0xAA), (b | 0x55). Several fixtures in the
 * range: on a DMX line their responses would collide, so send one with a
 * wrong checksum and the controller narrows the range.
 */
static void send_discovery(dmx_t* port, const uid_t uid, const bool collision) {
    uint8_t* r = &reply[1];
    uint8_t n = 0;
    reply[0] = 0;
    for (uint8_t i = 0; i < 7; i++) r[n++] = 0xFE;
    r[n++] = 0xAA;
    uint16_t sum = 0;
    for (uint8_t i = 0; i < 6; i++) {
        const uint8_t b = (uint8_t)(uid >> (8 * (5 - i)));
        r[n++] = b | 0xAA;
        r[n++] = b | 0x55;
        sum += (uint16_t)(b | 0xAA) + (uint16_t)(b | 0x55);
    }
    if (collision) sum ^= 0xFFFF;
    r[n++] = (sum >> 8) | 0xAA;
    r[n++] = (sum >> 8) | 0x55;
    r[n++] = (uint8_t)sum | 0xAA;
    r[n++] = (uint8_t)sum | 0x55;
    dmx_send(port, DMX_LABEL_RECEIVED, reply, 1 + n);
}

static void handle_discovery(dmx_t* port, const uint8_t* req, const uid_t dest, const uint16_t pid) {
    const uint8_t pdl = req[O_PDL];

    if (pid == PID_DISC_UNIQUE_BRANCH) {
        if (pdl != 12) { send_timeout(port); return; }
        const uid_t lower = get_uid(&req[O_PD]);
        const uid_t upper = get_uid(&req[O_PD + 6]);
        uint16_t found = 0;
        uid_t uid = 0;
        for (uint16_t i = 0; i < FIXTURES_MAX; i++) {
            const fixture_t* fix = fixtures_at(i);
            if (fix == NULL || fix->rdm_muted) continue;
            const uid_t u = fixture_uid(fix);
            if (u >= lower && u <= upper) { found++; uid = u; }
        }
        if (found == 0) send_timeout(port);
        else send_discovery(port, uid, found > 1);
        return;
    }

    if (pid != PID_DISC_MUTE && pid != PID_DISC_UN_MUTE) { send_timeout(port); return; }
    const bool mute = (pid == PID_DISC_MUTE);
    const bool broadcast = (dest == UID_BROADCAST_ALL || dest == UID_BROADCAST_MANUF);
    for (uint16_t i = 0; i < FIXTURES_MAX; i++) {
        fixture_t* fix = fixtures_at(i);
        if (fix == NULL || !(broadcast || fixture_uid(fix) == dest)) continue;
        fix->rdm_muted = mute;
        if (!broadcast) {
            const uint8_t control[2] = { 0, 0 };    /* control field: no flags */
            send_response(port, req, fix, RT_ACK, control, sizeof(control));
            return;
        }
    }
    send_timeout(port);     /* broadcasts are never answered */
}

static void handle_get(dmx_t* port, const uint8_t* req, const fixture_t* fix, const uint16_t pid, const uint32_t now_ms) {
    uint8_t pd[RDM_PD_MAX];
    uint8_t* p = pd;
    const uint8_t pdl = req[O_PDL];

    switch (pid) {
        case PID_SUPPORTED_PARAMS:
            p = put16(p, PID_DEVICE_MODEL_DESC);
            p = put16(p, PID_MANUFACTURER_LABEL);
            p = put16(p, PID_DMX_PERSONALITY);
            p = put16(p, PID_DMX_PERSONALITY_DESC);
            p = put16(p, PID_SENSOR_DEFINITION);
            p = put16(p, PID_SENSOR_VALUE);
            p = put16(p, RDM_PID_LAST_SEEN);
            break;
        case PID_DEVICE_INFO:
            p = put16(p, 0x0100);                   /* RDM protocol 1.0 */
            p = put16(p, 0x0001);                   /* device model */
            p = put16(p, PRODUCT_CATEGORY_FIXTURE);
            p = put32(p, fix->hb.fw_version);
            p = put16(p, sgRF_footprint(fix->hb.personality));
            *p++ = fix->hb.personality;
            *p++ = SGRF_PERSONALITY_COUNT;
            p = put16(p, fix->hb.dmx_addr);
            p = put16(p, 0);                        /* sub devices */
            *p++ = SENSOR_COUNT;
            break;
        case PID_DEVICE_MODEL_DESC:
            p = putstr(p, "seegrid module");
            break;
        case PID_MANUFACTURER_LABEL:
            p = putstr(p, "seegrid");
            break;
        case PID_SOFTWARE_VERSION:
            p = putstr(p, "fw ");
            *p++ = '0' + (fix->hb.fw_version % 10);
            break;
        case PID_DMX_START_ADDRESS:
            p = put16(p, fix->hb.dmx_addr);
            break;
        case PID_DMX_PERSONALITY:
            *p++ = fix->hb.personality;
            *p++ = SGRF_PERSONALITY_COUNT;
            break;
        case PID_DMX_PERSONALITY_DESC: {
            if (pdl != 1) { send_nack(port, req, fix, NR_FORMAT_ERROR); return; }
            const uint8_t pers = req[O_PD];
            if (!sgRF_footprint(pers)) { send_nack(port, req, fix, NR_DATA_OUT_OF_RANGE); return; }
            *p++ = pers;
            p = put16(p, sgRF_footprint(pers));
            p = putstr(p, pers == SGRF_PERSONALITY_16BIT ? "WRBG 16 bit" : "WRBG 8 bit");
            break;
        }
        case PID_IDENTIFY_DEVICE: {
            bool on = (fix->hb.flags & SGRF_HB_IDENTIFY) != 0;
            if (fix->pending_tx && fix->pending.identify != SGRF_CFG_KEEP_IDENTIFY) on = fix->pending.identify;
            *p++ = on;
            break;
        }
        case PID_SENSOR_DEFINITION:
        case PID_SENSOR_VALUE: {
            if (pdl != 1) { send_nack(port, req, fix, NR_FORMAT_ERROR); return; }
            const uint8_t sensor = req[O_PD];
            if (sensor >= SENSOR_COUNT) { send_nack(port, req, fix, NR_DATA_OUT_OF_RANGE); return; }
            *p++ = sensor;
            if (pid == PID_SENSOR_VALUE) {
                const uint16_t v = (sensor == SENSOR_BATTERY) ? fix->hb.battery_mv : fix->hb.rx_fps;
                p = put16(p, v);
                p = put16(p, 0);                    /* lowest, highest, recorded: not supported */
                p = put16(p, 0);
                p = put16(p, 0);
            }
            else if (sensor == SENSOR_BATTERY) {
                *p++ = SENS_VOLTAGE; *p++ = UNITS_VOLTS_DC; *p++ = PREFIX_MILLI;
                p = put16(p, 0); p = put16(p, 10000);       /* range */
                p = put16(p, 4400); p = put16(p, 6400);     /* normal: 4 AA cells */
                *p++ = 0;                                   /* no recorded values */
                p = putstr(p, "Battery");
            }
            else {
                *p++ = SENS_OTHER; *p++ = UNITS_NONE; *p++ = PREFIX_NONE;
                p = put16(p, 0); p = put16(p, 255);
                p = put16(p, 25); p = put16(p, 255);
                *p++ = 0;
                p = putstr(p, "DMX frames/s");
            }
            break;
        }
        case RDM_PID_LAST_SEEN:
            p = put32(p, now_ms - fix->last_seen_ms);
            break;
        default:
            send_nack(port, req, fix, NR_UNKNOWN_PID);
            return;
    }
    send_response(port, req, fix, RT_ACK, pd, (uint8_t)(p - pd));
}

static void handle_set(dmx_t* port, const uint8_t* req, fixture_t* fix, const uint16_t pid) {
    const uint8_t pdl = req[O_PDL];
    const uint8_t* pd = &req[O_PD];

    switch (pid) {
        case PID_DMX_START_ADDRESS: {
            if (pdl != 2) { send_nack(port, req, fix, NR_FORMAT_ERROR); return; }
            const uint16_t addr = ((uint16_t)pd[0] << 8) | pd[1];
            if (!sgRF_dmxAddressValid(addr, sgRF_footprint(fix->hb.personality))) { send_nack(port, req, fix, NR_DATA_OUT_OF_RANGE); return; }
            fixtures_queueConfig(fix, addr, SGRF_CFG_KEEP_IDENTIFY, SGRF_CFG_KEEP_PERSONALITY);
            fix->hb.dmx_addr = addr;    /* the next heartbeat confirms or corrects it */
            break;
        }
        case PID_DMX_PERSONALITY: {
            if (pdl != 1) { send_nack(port, req, fix, NR_FORMAT_ERROR); return; }
            const uint8_t pers = pd[0];
            /* the wider footprint must still fit the RF chunk at the current address */
            if (!sgRF_footprint(pers) || !sgRF_dmxAddressValid(fix->hb.dmx_addr, sgRF_footprint(pers))) {
                send_nack(port, req, fix, NR_DATA_OUT_OF_RANGE);
                return;
            }
            fixtures_queueConfig(fix, SGRF_CFG_KEEP_ADDR, SGRF_CFG_KEEP_IDENTIFY, pers);
            fix->hb.personality = pers;
            break;
        }
        case PID_IDENTIFY_DEVICE:
            if (pdl != 1) { send_nack(port, req, fix, NR_FORMAT_ERROR); return; }
            if (pd[0] > 1) { send_nack(port, req, fix, NR_DATA_OUT_OF_RANGE); return; }
            fixtures_queueConfig(fix, SGRF_CFG_KEEP_ADDR, pd[0], SGRF_CFG_KEEP_PERSONALITY);
            break;
        case PID_SUPPORTED_PARAMS: case PID_DEVICE_INFO: case PID_DEVICE_MODEL_DESC:
        case PID_MANUFACTURER_LABEL: case PID_SOFTWARE_VERSION: case PID_SENSOR_DEFINITION:
        case PID_SENSOR_VALUE: case RDM_PID_LAST_SEEN: case PID_DMX_PERSONALITY_DESC:
            send_nack(port, req, fix, NR_UNSUPPORTED_CC);
            return;
        default:
            send_nack(port, req, fix, NR_UNKNOWN_PID);
            return;
    }
    send_response(port, req, fix, RT_ACK, NULL, 0);
}

void rdm_handle(dmx_t* port, const uint8_t* req, const uint16_t len, const bool discovery, const uint32_t now_ms) {
    (void)discovery;    /* label 11 vs 7 only tells how a widget listens on the line */

    if (len < RDM_HEADER_LEN + 2 || req[0] != RDM_SC || req[1] != RDM_SUB_SC) { send_timeout(port); return; }
    const uint8_t msg_len = req[O_MSG_LEN];
    if (msg_len < RDM_HEADER_LEN || msg_len + 2 > len || req[O_PDL] != msg_len - RDM_HEADER_LEN
        || checksum(req, msg_len) != (((uint16_t)req[msg_len] << 8) | req[msg_len + 1])) {
        send_timeout(port);     /* a responder ignores broken packets */
        return;
    }

    const uid_t dest = get_uid(&req[O_DEST]);
    const uint8_t cc = req[O_CC];
    const uint16_t pid = ((uint16_t)req[O_PID] << 8) | req[O_PID + 1];

    if (cc == CC_DISC) { handle_discovery(port, req, dest, pid); return; }

    /* GET / SET to a single fixture; broadcast GET/SET are not answered (and not applied) */
    fixture_t* fix = NULL;
    for (uint16_t i = 0; i < FIXTURES_MAX && fix == NULL; i++) {
        fixture_t* f = fixtures_at(i);
        if (f != NULL && fixture_uid(f) == dest) fix = f;
    }
    if (fix == NULL || (cc != CC_GET && cc != CC_SET)) { send_timeout(port); return; }
    if (req[O_SUBDEV] != 0 || req[O_SUBDEV + 1] != 0) { send_nack(port, req, fix, NR_SUB_DEVICE_RANGE); return; }

    if (cc == CC_GET) handle_get(port, req, fix, pid, now_ms);
    else handle_set(port, req, fix, pid);
}
