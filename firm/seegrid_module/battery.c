#include "battery.h"

/* the XMC1100 VADC has no group registers: background scan into GLOBRES */
#define BATT_ADC_GROUP_NUM  BATT_V_ADC_GR_NUM
#define BATT_ADC_CH         1U          /* P2.8 = G0CH1 */
#define BATT_SAMPLES        8U
#define BATT_DIVIDER        2U          /* R6 = R5 = 51k */
#define ADC_REF_MV          3300UL      /* VDDP */
#define ADC_FULL_SCALE      4096UL      /* 12 bit */
#define ADC_TIMEOUT         1000U

void battLvl_init(battLvl_t* batt){
    /* XMC_VADC_GLOBAL_Init runs in cybsp_init (config tool) */
    XMC_VADC_GLOBAL_StartupCalibration(BATT_V_ADC_HW);
    XMC_VADC_GLOBAL_BackgroundInit(BATT_V_ADC_HW, &BATT_V_ADC_background_scan_config);
    XMC_VADC_GLOBAL_BackgroundAddChannelToSequence(BATT_V_ADC_HW, BATT_ADC_GROUP_NUM, BATT_ADC_CH);
    batt->init = true;
    batt->last = 0;
}

/* one conversion, 12 bit; 0xFFFF on timeout */
static uint16_t _battLvl_convert(void) {
    XMC_VADC_GLOBAL_BackgroundTriggerConversion(BATT_V_ADC_HW);
    for (uint16_t i = 0; i < ADC_TIMEOUT; i++) {
        const uint32_t res = XMC_VADC_GLOBAL_GetDetailedResult(BATT_V_ADC_HW);
        if (res & VADC_GLOBRES_VF_Msk) {
            /* right aligned: the alignment is a channel setting (CHCTR.RESPOS),
               which the XMC1100 background scan leaves at its default */
            return (uint16_t)(res & 0x0FFFU);
        }
    }
    return 0xFFFF;
}

uint16_t battLvl_read(battLvl_t* batt){
    if (!batt->init) return 0;
    uint32_t sum = 0;
    for (uint8_t i = 0; i < BATT_SAMPLES; i++) {
        const uint16_t raw = _battLvl_convert();
        if (raw == 0xFFFF) return batt->last;
        sum += raw;
    }
    batt->last = (uint16_t)(sum * ADC_REF_MV * BATT_DIVIDER / (BATT_SAMPLES * ADC_FULL_SCALE));
    return batt->last;
}

uint16_t battLvl_lastRead(const battLvl_t* batt){
    return batt->last;
}
