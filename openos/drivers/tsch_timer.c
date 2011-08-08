/*
 * Driver for the TSCH timers.
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
 *
 * These timers are dedicated to TSCH. TSCH uses TimerA, clocked from the 32kHz
 * crystal, in up mode, with interrupts enabled. The counter resetting
 * corresponds to the slot edge.
 *
 * TSCH only uses the following registers:
 * - CCR0 holds the slot duration; it will cause the counter to reset
 * - CCR1 is used in compare mode to time the MAC FSM
 * - CCR2 is used in capture mode to timestamp the arrival of a packet for
 *   synchronization.
 */

#include "msp430x26x.h"
#include "tsch_timer.h"
#include "IEEE802154e.h"

//=========================== variables ===========================================

//=========================== prototypes ==========================================

//=========================== public ==============================================

void tsch_timer_init() {
   
   BCSCTL3 |= LFXT1S_0;                          // source ACLK from 32kHz crystal
   
   // CCR0 contains max value of counter (slot length)
   TACCTL0  =  CCIE;
   TACCR0   =  SLOT_TIME;
   
   // CCR1 in compare mode
   TACCTL1  =  0;
   TACCR1   =  0;
   
   // CCR2 in capture mode
   TACCTL2  =  0;
   TACCR2   =  0;
   
   //reset couter
   TAR      =  0;
   
   // start counting
   TACTL    =  MC_1+TBSSEL_1;                    // up mode, clocked from ACLK
}

void tsch_timer_schedule(uint16_t offset) {
   TACCR1   =  offset;                           // offset when to fire
   TACCTL1  =  CCIE;                             // enable CCR1 interrupt
}

void tsch_timer_cancel() {
   TACCR1   =  0;                                // reset CCR1 value (not really necessary)
   TACCTL1 &= ~CCIE;                             // disable CCR1 interrupt
}




void enable_capture(uint8_t timer_id){
   //poipoi expand to include other timer, we only do timer5 now for tsch
   if(timer_id == 5)
   {
      TBCCTL5 = CAP|SCS|CCIS1|CM_3;
   }
}

