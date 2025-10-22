#ifndef BATTERY_H
#define BATTERY_H

#include "cybsp.h"

#include "stdint.h"
#include "stdbool.h"

typedef struct {
    XMC_GPIO_PORT_t*    port;
    uint8_t             pin;
    bool                init;
    uint16_t            last;
} battLvl_t;


void battLvl_init(battLvl_t* batt);
uint16_t battLvl_read(battLvl_t* batt);
uint16_t battLvl_lastRead(const battLvl_t* batt);

#endif