#ifndef FIXTURES_H
#define FIXTURES_H

#include "stdint.h"
#include "stdbool.h"
#include "seeGridRF.h"

/**
 * Fixtures known to the gateway, filled from their heartbeats.
 * Config changes are sent a few times after DMX frames (no ACK over the air),
 * the next heartbeat shows whether they arrived.
 */

#define FIXTURES_MAX            128
#define FIXTURE_CONFIG_REPEAT   5

typedef struct {
    bool                used;
    sgRF_heartbeat_t    hb;             /* last heartbeat (id, address, battery, ...) */
    uint32_t            last_seen_ms;
    bool                rdm_muted;      /* RDM discovery state */
    sgRF_config_t       pending;        /* config to (re)send */
    uint8_t             pending_tx;     /* remaining transmissions of `pending` */
} fixture_t;

void fixtures_heartbeat(const sgRF_heartbeat_t* hb, const uint32_t now_ms);

fixture_t* fixtures_find(const uint32_t id);

/* index based iteration over the used entries: 0 .. FIXTURES_MAX-1, NULL if unused */
fixture_t* fixtures_at(const uint16_t index);

/**
 * queue a config change; dmx_addr / identify may be SGRF_CFG_KEEP_*
 */
void fixtures_queueConfig(fixture_t* fix, const uint16_t dmx_addr, const uint8_t identify);

/**
 * next config packet to broadcast (round robin over the pending ones), NULL if none
 */
const sgRF_config_t* fixtures_nextConfig(void);

#endif
