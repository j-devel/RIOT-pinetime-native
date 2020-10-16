/*
 * Copyright (C) 2018 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#ifdef USE_BOARD_NATIVE
#include <time.h>
#endif

#include "cpu.h"

#ifndef USE_BOARD_NATIVE
#include "nrf_clock.h"
#endif

#include "controller.h"
#include "controller/time.h"

#ifndef USE_BOARD_NATIVE
#define CONTROLLER_TIME_RTC     NRF_RTC2
#define CONTROLLER_TIME_ISR     isr_rtc2
#define CONTROLLER_TIME_IRQn    RTC2_IRQn
#endif

#define COUNTER_MASK            0x00FFFFFF /* 24 bit counter */

/* some constants from tzfile.h */

#define SECSPERMIN      60
#define MINSPERHOUR     60
#define HOURSPERDAY     24
#define DAYSPERWEEK     7
#define DAYSPERNYEAR    365
#define DAYSPERLYEAR    366
#define SECSPERHOUR     (SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY      ((long) SECSPERHOUR * HOURSPERDAY)
#define MONSPERYEAR     12
#define TM_YEAR_BASE    1900
#define isleap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)

#ifndef USE_BOARD_NATIVE
static const unsigned mon_lengths[2][MONSPERYEAR] = {
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};
#endif

static const char *mon_short_names[MONSPERYEAR + 1] = {
    [0] = "Inv",
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "June",
    "July",
    "Aug",
    "Sept",
    "Oct",
    "Nov",
    "Dec",
};

static const char *mon_long_names[MONSPERYEAR + 1] = {
    [0] = "Invalid",
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December",
};

#ifndef USE_BOARD_NATIVE
void controller_time_rtc_inc_compare(void)
{
    /* TODO: add fracs compensation support */
    uint32_t cur_counter = CONTROLLER_TIME_RTC->COUNTER;

    /* Increment to the next 256 multiple */
    CONTROLLER_TIME_RTC->CC[0] = (cur_counter + 256) & ~(255);
}

static void controller_time_match_callback(void)
{
	controller_t *controller = controller_get();
    thread_flags_set((thread_t*)sched_threads[controller->pid], CONTROLLER_THREAD_FLAG_TICK);
	controller_time_rtc_inc_compare();
}

void controller_time_rtc_start(void)
{
	CONTROLLER_TIME_RTC->TASKS_START = 1;
}

void controller_time_rtc_stop(void)
{
	CONTROLLER_TIME_RTC->TASKS_STOP = 1;
}

void controller_time_rtc_set_prescaler(uint32_t prescaler)
{
	CONTROLLER_TIME_RTC->PRESCALER = prescaler;
}

uint32_t controller_time_get_ticks(void)
{
	return CONTROLLER_TIME_RTC->COUNTER;
}

void CONTROLLER_TIME_ISR(void)
{
    if (CONTROLLER_TIME_RTC->EVENTS_COMPARE[0] == 1) {
        CONTROLLER_TIME_RTC->EVENTS_COMPARE[0] = 0;
        controller_time_match_callback();
    }
    cortexm_isr_end();
}
#endif

const char *controller_time_month_get_long_name(controller_time_spec_t *time)
{
    assert(time->month < MONSPERYEAR);
    return mon_long_names[time->month];
}

const char *controller_time_month_get_short_name(controller_time_spec_t *time)
{
    assert(time->month < MONSPERYEAR);
    return mon_short_names[time->month];
}

void controller_time_set_time(controller_t *controller, controller_time_spec_t *time)
{
    /* TODO: take fractionals into account */
    memcpy(&controller->cur_time, time, sizeof(controller_time_spec_t));
}

const controller_time_spec_t *controller_time_get_time(controller_t *controller)
{
    return &controller->cur_time;
}

#ifdef USE_BOARD_NATIVE
void controller_update_time_native(controller_t *controller)
{
    struct tm now;
    time_t clock = time(NULL);
    localtime_r(&clock, &now);

    controller_time_spec_t *ts = &controller->cur_time;
    ts->second = now.tm_sec;
    ts->minute = now.tm_min;
    ts->hour = now.tm_hour;
    ts->dayofmonth = now.tm_mday;
    ts->month = now.tm_mon + 1;
    ts->year = now.tm_year + 1900;
}
#else
void controller_update_time(controller_t *controller)
{
    uint32_t counter = CONTROLLER_TIME_RTC->COUNTER;
    uint32_t fractionals = counter - controller->last_update;
    controller->last_update = counter;
    unsigned seconds = (fractionals/256);

    controller_time_spec_t *ts = &controller->cur_time;

    ts->second += seconds;
    while (ts->second >= SECSPERMIN) {
        ts->second -= SECSPERMIN;
        ts->minute++;
        if (ts->minute >= MINSPERHOUR) {
            ts->minute = 0;
            ts->hour++;
            if (ts->hour >= HOURSPERDAY) {
                ts->hour = 0;
                ts->dayofmonth++;
                if (ts->dayofmonth > mon_lengths[isleap(ts->year)][ts->month]) {
                    ts->dayofmonth = 1;
                    ts->month++;
                    if (ts->month > MONSPERYEAR) {
                        ts->year++;
                        ts->month = 1;
                    }
                }
            }
        }
    }
}

void controller_time_init(void)
{
    controller_time_rtc_stop();
	NVIC_EnableIRQ(CONTROLLER_TIME_IRQn);

	controller_time_rtc_set_prescaler(CONTROLLER_TIME_PRESCALER);
    CONTROLLER_TIME_RTC->INTENSET = RTC_INTENSET_COMPARE0_Msk;

    CONTROLLER_TIME_RTC->CC[0] = 0;
	controller_time_rtc_inc_compare();
	controller_time_rtc_start();
}
#endif
