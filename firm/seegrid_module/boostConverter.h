#ifndef BOOSTCONVERTER_H
#define BOOSTCONVERTER_H

#include "cybsp.h"
#include "stdbool.h"

typedef struct {
    XMC_GPIO_PORT_t*    port;
    uint8_t             pin;
    bool                init;
} boostCon_t;

void boostCon_init(boostCon_t* boost);
void boostCon_set(bool set);
void boostCon_setOff();
void bosstCon_setOn();
bool boostCon_get();

#endif