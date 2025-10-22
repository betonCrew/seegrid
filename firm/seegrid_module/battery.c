#include "battery.h"

void battLvl_init(battLvl_t* batt){

}

uint16_t battLvl_read(battLvl_t* batt){
    uint16_t adcVal;
    // TODO turn ADC on
    // TODO read samples
    adcVal = 0;
    // TODO store result
    return adcVal;
}

uint16_t battLvl_lastRead(const battLvl_t* batt){
    return batt->last;
}