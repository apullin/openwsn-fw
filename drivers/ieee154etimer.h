/*
 * Driver for the IEEE802.15.4e timers.
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
 */

#ifndef __IEEE154E_TIMER_H
#define __IEEE154E_TIMER_H

#include "openwsn.h"

// this is a workaround from the fact that the interrupt pin for the radio is
// not connected to a pin on the MSP which allows time capture.
#define CAPTURE_TIME() DEBUG_PIN_FRAME_SET(); \
                       TACCTL2 |=  CCIS0;     \
                       TACCTL2 &= ~CCIS0;     \
                       DEBUG_PIN_FRAME_CLR();

typedef struct timestamp_t {
   bool      valid;
   uint16_t  timestamp;
} timestamp_t;

// callable functions
void ieee154e_timer_init();
// compare
void ieee154e_timer_schedule(uint16_t offset);
void ieee154e_timer_cancel();

void ieee154e_timer_clearCaptureOverflow();
void ieee154e_timer_enableCaptureInterrupt();
void ieee154e_timer_getCapturedTime(timestamp_t* timestampToWrite);
void ieee154e_timer_disableCaptureInterrupt();

#endif
