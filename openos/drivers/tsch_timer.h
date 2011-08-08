/*
 * Driver for the TSCH timers.
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
 */

#ifndef __TSCH_TIMER_H
#define __TSCH_TIMER_H

#include "openwsn.h"

// callable functions
void tsch_timer_init();
void tsch_timer_schedule(uint16_t offset);
void tsch_timer_cancel();

// functions to call when timer fires
#ifdef OPENWSN_STACK
void timer_mac_periodic_fired();
#endif

#endif
