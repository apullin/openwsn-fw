 /**
\brief Driver for the IEEE802.15.4e timers.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
*/

#ifndef __IEEE154ETIMER_H
#define __IEEE154ETIMER_H

/**
\addtogroup MAClow
\{
\addtogroup IEEE802154Etimer
\{
*/

#include "openwsn.h"

//=========================== define ==========================================

// this is a workaround from the fact that the interrupt pin for the radio is
// not connected to a pin on the MSP which allows time capture.
#define CAPTURE_TIME() TACCTL2 |=  CCIS0;     \
                       TACCTL2 &= ~CCIS0;

//=========================== typedef =========================================

typedef struct {
   bool      valid;
   uint16_t  timestamp;
} timestamp_t;

//=========================== variables =======================================

//=========================== prototypes ======================================

void     ieee154etimer_init();
void     ieee154etimer_schedule(uint16_t offset);
void     ieee154etimer_cancel();
uint16_t ieee154etimer_getCapturedTime();

/**
\}
\}
*/

#endif
