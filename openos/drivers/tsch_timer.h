/*
 * Driver for the TSCH timers.
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
 */

#ifndef __TSCH_TIMER_H
#define __TSCH_TIMER_H

#include "openwsn.h"

// this is a workaround from the fact that the interrupt pin for the radio is
// not connected to a pin on the MSP which allows time capture.
#define CAPTURE_TIME TACCTL2 ^= CCIS0

typedef struct timestamp_t {
   bool      valid;
   uint16_t  timestamp;
} timestamp_t;

// callable functions
void tsch_timer_init();
void tsch_timer_schedule(uint16_t offset);
void tsch_timer_cancel();
void read_capture(timestamp_t* timestampToWrite);

// functions to call when timer fires
#ifdef OPENWSN_STACK
void timer_mac_periodic_fired();
#endif

#endif
