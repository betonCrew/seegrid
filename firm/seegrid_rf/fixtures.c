#include "fixtures.h"
#include <string.h>

static fixture_t fixtures[FIXTURES_MAX];

fixture_t* fixtures_find(const uint32_t id) {
    for (uint16_t i = 0; i < FIXTURES_MAX; i++) {
        if (fixtures[i].used && fixtures[i].hb.id == id) return &fixtures[i];
    }
    return NULL;
}

fixture_t* fixtures_at(const uint16_t index) {
    if (index >= FIXTURES_MAX || !fixtures[index].used) return NULL;
    return &fixtures[index];
}

void fixtures_heartbeat(const sgRF_heartbeat_t* hb, const uint32_t now_ms) {
    fixture_t* fix = fixtures_find(hb->id);
    if (fix == NULL) {
        for (uint16_t i = 0; i < FIXTURES_MAX && fix == NULL; i++) {
            if (!fixtures[i].used) fix = &fixtures[i];
        }
        if (fix == NULL) return;    /* table full */
        memset(fix, 0, sizeof(*fix));
        fix->used = true;
    }
    fix->hb = *hb;
    fix->last_seen_ms = now_ms;
    /* the fixture reports what it applied: stop repeating a config it already has */
    if (fix->pending_tx
        && (fix->pending.dmx_addr == SGRF_CFG_KEEP_ADDR || fix->pending.dmx_addr == hb->dmx_addr)
        && (fix->pending.identify == SGRF_CFG_KEEP_IDENTIFY
            || (fix->pending.identify != 0) == ((hb->flags & SGRF_HB_IDENTIFY) != 0))) {
        fix->pending_tx = 0;
    }
}

void fixtures_queueConfig(fixture_t* fix, const uint16_t dmx_addr, const uint8_t identify) {
    static uint8_t seq;
    if (fix->pending_tx == 0) {
        memset(&fix->pending, 0, sizeof(fix->pending));
        fix->pending.type = SGRF_PKT_CONFIG;
        fix->pending.id = fix->hb.id;
        fix->pending.dmx_addr = SGRF_CFG_KEEP_ADDR;
        fix->pending.identify = SGRF_CFG_KEEP_IDENTIFY;
    }
    /* merge with a change that is still being sent */
    if (dmx_addr != SGRF_CFG_KEEP_ADDR) fix->pending.dmx_addr = dmx_addr;
    if (identify != SGRF_CFG_KEEP_IDENTIFY) fix->pending.identify = identify;
    fix->pending.seq = seq++;
    fix->pending_tx = FIXTURE_CONFIG_REPEAT;
}

const sgRF_config_t* fixtures_nextConfig(void) {
    static uint16_t next;
    for (uint16_t n = 0; n < FIXTURES_MAX; n++) {
        fixture_t* fix = &fixtures[next];
        next = (next + 1) % FIXTURES_MAX;
        if (fix->used && fix->pending_tx) {
            fix->pending_tx--;
            return &fix->pending;
        }
    }
    return NULL;
}
