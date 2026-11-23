#ifndef BATTERY_H
#define BATTERY_H

#include "cybsp.h"

#include "stdint.h"
#include "stdbool.h"

/**
 * Battery voltage on BATT_V (P2.8 = VADC G0CH1, configured in design.modus),
 * divided 1:2 by R6/R5. The ADC reference is VDDP (3.3 V from the MCP1700).
 */
typedef struct {
    bool                init;
    uint16_t            last;       /* [mV] */
} battLvl_t;


void battLvl_init(battLvl_t* batt);

/**
 * measure (blocking, a few us) and return the battery voltage [mV]
 */
uint16_t battLvl_read(battLvl_t* batt);
uint16_t battLvl_lastRead(const battLvl_t* batt);

#endif
