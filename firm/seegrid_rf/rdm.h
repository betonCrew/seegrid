#ifndef RDM_H
#define RDM_H

#include "stdint.h"
#include "stdbool.h"
#include "dmx.h"

/**
 * RDM (ANSI E1.20) proxy: answers requests from the host on behalf of the
 * fixtures in the fixture table. Values come from the last heartbeat, SET
 * commands become config packets (see fixtures.h).
 *
 * UID = RDM_MANUFACTURER_ID : fixture id
 */
#define RDM_MANUFACTURER_ID     0x7FF0  /* ESTA prototype range, until seegrid has its own */
#define RDM_PID_LAST_SEEN       0x8000  /* manufacturer specific: ms since the last heartbeat (u32) */

/**
 * handle one request (Enttec label 7 / 11 payload: RDM packet from the start
 * code to the checksum) and send the reply (label 5) or a timeout (label 12)
 */
void rdm_handle(dmx_t* port, const uint8_t* req, const uint16_t len, const bool discovery, const uint32_t now_ms);

#endif
