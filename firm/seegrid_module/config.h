#ifndef CONFIG_H
#define CONFIG_H

#include "cybsp.h"
#include "stdint.h"
#include "stdbool.h"

/**
 * Fixture configuration, kept in the last flash page (reserved in linker.ld)
 */
typedef struct {
    uint16_t    dmx_addr;       /* DMX start address 1..512 */
    uint8_t     personality;    /* SGRF_PERSONALITY_* (8 / 16 bit channels) */
} config_t;

/**
 * fixture id: CRC32 of the 16 byte unique chip id
 */
uint32_t config_fixtureId(void);

/**
 * load the stored configuration, defaults if none is stored
 */
void config_load(config_t* cfg);

/**
 * store the configuration; false if the flash readback differs
 */
bool config_save(const config_t* cfg);

/**
 * address and personality the fixture can work with (see sgRF_dmxAddressValid)
 */
bool config_valid(const uint16_t dmx_addr, const uint8_t personality);

#endif
