/*
 * Driver for the timers.
 *
 * Authors:
 * Ankur Mehta <watteyne@eecs.berkeley.edu>, October 2010
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef __TIMERS_H
#define __TIMERS_H

#include "openwsn.h"

//timer ids
enum {
#ifdef OPENWSN_STACK
   TIMER_MAC_PERIODIC = 0,                       // mapped onto timerB CCR0
   TIMER_MAC_BACKOFF  = 1,                       // mapped onto timerB CCR1
   TIMER_MAC_WATCHDOG = 2,                       // mapped onto timerB CCR2
   TIMER_TCP_TIMEOUT  = 3,                       // mapped onto timerB CCR3
   TIMER_RPL          = 4,                       // mapped onto timerB CCR4
#endif
   TIMER_B5 = 5,                                 // mapped onto timerB CCR5
#ifdef TASK_APPLICATION
   TIMER_TASK_APPLICATION = 6,                   // mapped onto timerB CCR6
#else
   TIMER_B6 = 6,                                 // mapped onto timerB CCR5
#endif
};

#ifdef TIMER_A_ENABLED
#define TIMER_COUNT 13
#else
#define TIMER_COUNT 7
#endif

extern uint16_t timers_period[TIMER_COUNT];
extern bool     timers_continuous[TIMER_COUNT];

// initialize the timers
void timer_init();
// read/write individual 8-bit registers
void timer_start(uint8_t timer_id, uint16_t duration, bool continuous);
void timer_startOneShot(uint8_t timer_id, uint16_t duration);
void timer_startPeriodic(uint8_t timer_id, uint16_t duration);
void timer_stop(uint8_t timer_id);
// functions to call when timer fires
#ifdef OPENWSN_STACK
void timer_mac_periodic_fired();
void timer_mac_backoff_fired();
void timer_mac_watchdog_fired();
void timer_tcp_timeout_fired();
void timer_rpl_fired();
#endif

#endif
