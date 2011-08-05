/*
 * Driver for the TSCH timers.
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
 */

#ifndef __MAC_TIMERS_H
#define __MAC_TIMERS_H

#include "openwsn.h"

// callable functions
void mac_timer_init();
void mac_timer_schedule(uint16_t offset);
void mac_timer_cancel();

// functions to call when timer fires
#ifdef OPENWSN_STACK
void timer_mac_periodic_fired();
#endif

#endif
