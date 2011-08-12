/*
 * Driver for the IEEE802.15.4e timers.
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
 */

#ifndef __IEEE154ETIMER_H
#define __IEEE154ETIMER_H

#include "openwsn.h"

// this is a workaround from the fact that the interrupt pin for the radio is
// not connected to a pin on the MSP which allows time capture.
#define CAPTURE_TIME() TACCTL2 |=  CCIS0;     \
                       TACCTL2 &= ~CCIS0;

typedef struct timestamp_t {
   bool      valid;
   uint16_t  timestamp;
} timestamp_t;

// callable functions
void ieee154etimer_init();
// compare
void ieee154etimer_schedule(uint16_t offset);
void ieee154etimer_cancel();

void ieee154etimer_clearCaptureOverflow();
void ieee154etimer_enableCaptureInterrupt();
void ieee154etimer_getCapturedTime(timestamp_t* timestampToWrite);
void ieee154etimer_disableCaptureInterrupt();

#endif
