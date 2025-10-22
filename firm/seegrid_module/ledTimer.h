#ifndef LEDTIMER_H
#define LEDTIMER_H

#include "cybsp.h"
#include "cy_utils.h"

#include "stdint.h"
#include "stdbool.h"

#include "ledFx.h"
#include "colors.h"

typedef uint8_t seconds_t;
typedef uint8_t minutes_t;



typedef uint8_t ch_t;

typedef struct {
    const minutes_t         play_time;      // [minutes]
    const seconds_t         fade_time;      // [seconds]
    const ledFx_t           fx;
    const color_preset_t    col;
} cue_t;

typedef struct {
    const cue_t*        data;
    const uint8_t       length;
    uint8_t             index;      // 0 - stop, 1..511 - index
} cueList_t;

void playCueList(const cueList_t* list, XMC_RTC_TIME_t time);
void nextCue(cueList_t* list);
XMC_RTC_STATUS_t setTime(const XMC_RTC_TIME_t* time);
XMC_RTC_STATUS_t setAlarm(const XMC_RTC_TIME_t* time);
void handleAlarm();
void loadCue();
void disableAlarmIrq();
void enableAlarmIrq();

#endif