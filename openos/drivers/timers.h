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
   TIMER_RPL                 = 0,                // mapped onto timerB CCR0
   TIMER_TCP_TIMEOUT         = 1,                // mapped onto timerB CCR1
#else
   TIMER_B0                  = 0,                // mapped onto timerB CCR0
   TIMER_B1                  = 1,                // mapped onto timerB CCR1
#endif
   TIMER_B2                  = 2,                // mapped onto timerB CCR2
   TIMER_B3                  = 3,                // mapped onto timerB CCR3
   TIMER_B4                  = 4,                // mapped onto timerB CCR4
   TIMER_B5                  = 5,                // mapped onto timerB CCR5
   TIMER_B6                  = 6,                // mapped onto timerB CCR6
   TIMER_COUNT               = 7,                // number of available timers
};

extern uint16_t timers_period[TIMER_COUNT];
extern bool     timers_continuous[TIMER_COUNT];

// callable functions
void timer_init();
void timer_start(uint8_t timer_id, uint16_t duration, bool continuous);
void timer_startOneShot(uint8_t timer_id, uint16_t duration);
void timer_startPeriodic(uint8_t timer_id, uint16_t duration);
void timer_stop(uint8_t timer_id);

// functions to call when timer fires
#ifdef OPENWSN_STACK
void timer_rpl_fired();
void timer_tcp_timeout_fired();
#endif

#endif
