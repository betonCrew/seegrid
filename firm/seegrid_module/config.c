#include "config.h"
#include "../seagridRF/seeGridRF.h"
#include "xmc_flash.h"
#include <string.h>
#include <stddef.h>

#define UCID_ADDR           0x10000FF0UL                /* 16 byte unique chip id */
#define UCID_LEN            16U
#define CONFIG_PAGE_ADDR    0x10008F00UL                /* last page of the 32 kB flash */
#define CONFIG_MAGIC        0x31434753UL                /* "SGC1" */
#define CONFIG_DEFAULT_ADDR 1U

/* one 16 byte flash block */
typedef struct {
    uint32_t    magic;
    uint16_t    dmx_addr;
    uint16_t    reserved;
    uint32_t    reserved2;
    uint32_t    crc;            /* CRC32 of the 12 bytes before */
} _config_block_t;

_Static_assert(sizeof(_config_block_t) == 16, "one flash block");

static uint32_t _crc32(const uint8_t* data, uint32_t len) {
    uint32_t crc = 0xFFFFFFFFUL;
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++) {
            crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
        }
    }
    return ~crc;
}

uint32_t config_fixtureId(void) {
    return _crc32((const uint8_t*)UCID_ADDR, UCID_LEN);
}

bool config_dmxAddrValid(const uint16_t dmx_addr) {
    if (dmx_addr < 1 || dmx_addr > 512) return false;
    const uint16_t offset = (dmx_addr - 1) % BROADCAST_PL_WITDH;
    return offset + CONFIG_FOOTPRINT <= BROADCAST_PL_WITDH;
}

void config_load(config_t* cfg) {
    const _config_block_t* b = (const _config_block_t*)CONFIG_PAGE_ADDR;
    if (b->magic == CONFIG_MAGIC
        && b->crc == _crc32((const uint8_t*)b, offsetof(_config_block_t, crc))
        && config_dmxAddrValid(b->dmx_addr)) {
        cfg->dmx_addr = b->dmx_addr;
    }
    else {
        cfg->dmx_addr = CONFIG_DEFAULT_ADDR;
    }
}

bool config_save(const config_t* cfg) {
    static uint32_t page[XMC_FLASH_WORDS_PER_PAGE];
    memset(page, 0, sizeof(page));
    _config_block_t* b = (_config_block_t*)page;
    b->magic = CONFIG_MAGIC;
    b->dmx_addr = cfg->dmx_addr;
    b->crc = _crc32((const uint8_t*)b, offsetof(_config_block_t, crc));

    XMC_FLASH_ErasePage((uint32_t*)CONFIG_PAGE_ADDR);
    XMC_FLASH_ProgramPage((uint32_t*)CONFIG_PAGE_ADDR, page);
    return memcmp((const void*)CONFIG_PAGE_ADDR, page, sizeof(page)) == 0;
}
