#include "ledTimer.h"
#include "ledLib.h"

extern cueList_t* currentCueList;
extern led_t led;

static void addMinutes(XMC_RTC_TIME_t* time, minutes_t min) {
    // FIXME days können nicht überlaufen, aber wir haben heute sowieso den 1. des Monats!
    uint8_t minutes = time->minutes + min;
    uint8_t hours = time->hours + (minutes / 60);
    uint8_t days = time->days + (hours / 24);
    time->minutes = minutes % 60;
    time->hours = hours % 24;
    time->days = days;
}

void nextCue(cueList_t* list) {
    if (++list->index > (list->length-1)) list->index=0;
    const cue_t* next = list->data + (list->index*sizeof(cue_t));
    XMC_RTC_TIME_t next_time;
    XMC_RTC_GetTime(&next_time); 
    addMinutes(&next_time, next->play_time);
    setAlarm(&next_time);
}

XMC_RTC_STATUS_t setTime(const XMC_RTC_TIME_t* time) {
    if (XMC_RTC_IsRunning() && XMC_RTC_IsEnabled()) {
        while ((XMC_SCU_GetMirrorStatus() & (SCU_GENERAL_MIRRSTS_RTC_TIM0_Msk | SCU_GENERAL_MIRRSTS_RTC_TIM1_Msk)) != 0U)
        {} // check SCU_MIRRSTS to ensure that no transfer over serial interface is pending 
        RTC->TIM0 = time->raw0;
        RTC->TIM1 = time->raw1;
        return XMC_RTC_STATUS_OK;
    }
    else return XMC_RTC_STATUS_ERROR;
}

XMC_RTC_STATUS_t setAlarm(const XMC_RTC_TIME_t* time) {
    if (XMC_RTC_IsRunning() && XMC_RTC_IsEnabled()) {
        const XMC_RTC_ALARM_t alarm = {
            .seconds = 0,
            .minutes = time->minutes,
            .hours = time->hours,
            .days = time->days,
            .month = time->month,
            .year = time->year,
        };
        while ((XMC_SCU_GetMirrorStatus() & (SCU_GENERAL_MIRRSTS_RTC_ATIM0_Msk | SCU_GENERAL_MIRRSTS_RTC_ATIM1_Msk)) != 0U)
        {} // check SCU_MIRRSTS to ensure that no transfer over serial interface is pending
        RTC->ATIM0 = alarm.raw0;
        RTC->ATIM1 = alarm.raw1;
        return XMC_RTC_STATUS_OK;
    }
    else return XMC_RTC_STATUS_ERROR;
}

inline void disableAlarmIrq() {
    XMC_RTC_DisableEvent(XMC_RTC_EVENT_ALARM);
}

inline void enableAlarmIrq() {
    XMC_RTC_EnableEvent(XMC_RTC_EVENT_ALARM);
}