/*
 * Driver for the timers.
 *
 * Authors:
 * Ankur Mehta <watteyne@eecs.berkeley.edu>, October 2010
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#include "msp430x26x.h"
#include "timers.h"
#include "scheduler.h"

//=========================== variables ===========================================

uint16_t timers_period[TIMER_COUNT];
bool     timers_continuous[TIMER_COUNT];

//=========================== prototypes ==========================================

//=========================== public ==============================================

void timer_init() {
   uint8_t i;
   for(i=0;i<TIMER_COUNT;i++) {
      timer_start(i, 0, FALSE);
   }
   
   BCSCTL3 |= LFXT1S_0;                          // ACLK sources from external 32kHz

   //set CCRBx registers
   TBCCTL0  = 0;
   TBCCR0   = 0;
   TBCCTL1  = 0;
   TBCCR1   = 0;
   TBCCTL2  = 0;
   TBCCR2   = 0;
   TBCCTL3  = 0;
   TBCCR3   = 0;
   TBCCTL4  = 0;
   TBCCR4   = 0;
   TBCCTL5  = 0;
   TBCCR5   = 0;
   TBCCTL6  = 0;
   TBCCR6   = 0;

   //start TimerB on 32kHz ACLK
   TBCTL    = MC_2+TBSSEL_1;                     // continuous mode, using ACLK

#ifdef TIMER_A_ENABLED
   //set CCRAx registers
   TACCTL0  = 0;
   TACCR0   = 0;
   TACCTL1  = 0;
   TACCR1   = 0;
   TACCTL2  = 0;
   TACCR2   = 0;
   //start TimerA on 32kHz ACLK
   TACTL    = MC_2+TBSSEL_1;                     // continuous mode, using ACLK
#endif

}

void timer_startOneShot(uint8_t timer_id, uint16_t duration) {
   timer_start(timer_id, duration, FALSE);
}

void timer_startPeriodic(uint8_t timer_id, uint16_t duration) {
   timer_start(timer_id, duration, TRUE);
}

void timer_stop(uint8_t timer_id) {
   timers_period[timer_id] = 0;
   timers_continuous[timer_id] = 0;
   switch(timer_id) {
      case 0:
         TBCCR0   = 0;
         TBCCTL0 &= ~CCIE;
         break;
      case 1:
         TBCCR1   = 0;
         TBCCTL1 &= ~CCIE;
         break;
      case 2:
         TBCCR2   = 0;
         TBCCTL2 &= ~CCIE;
         break;
      case 3:
         TBCCR3   = 0;
         TBCCTL3 &= ~CCIE;
         break;
      case 4:
         TBCCR4   = 0;
         TBCCTL4 &= ~CCIE;
         break;
      case 5:
         TBCCR5   = 0;
         TBCCTL5 &= ~CCIE;
         break;
      case 6:
         TBCCR6   = 0;
         TBCCTL6 &= ~CCIE;
         break;
#ifdef TIMER_A_ENABLED
      case 10:
         TACCR0   = 0;
         TACCTL0 &= ~CCIE;
         break;
      case 11:
         TACCR1   = 0;
         TACCTL1 &= ~CCIE;
         break;
      case 12:
         TACCR2   = 0;
         TACCTL2 &= ~CCIE;
         break;
#endif
   }
}

//=========================== private =============================================

void timer_start(uint8_t timer_id, uint16_t duration, bool continuous) {
   timers_period[timer_id] = duration;
   timers_continuous[timer_id] = continuous;
   switch(timer_id) {
      case 0:
         TBCCR0   = TBR+timers_period[timer_id];
         TBCCTL0  = CCIE;
         break;
      case 1:
         TBCCR1   = TBR+timers_period[timer_id];
         TBCCTL1  = CCIE;
         break;
      case 2:
         TBCCR2   = TBR+timers_period[timer_id];
         TBCCTL2  = CCIE;
         break;
      case 3:
         TBCCR3   = TBR+timers_period[timer_id];
         TBCCTL3  = CCIE;
         break;
      case 4:
         TBCCR4   = TBR+timers_period[timer_id];
         TBCCTL4  = CCIE;
         break;
      case 5:
         TBCCR5   = TBR+timers_period[timer_id];
         TBCCTL5  = CCIE;
         break;
      case 6:
         TBCCR6   = TBR+timers_period[timer_id];
         TBCCTL6  = CCIE;
         break;
         
#ifdef TIMER_A_ENABLED
      case 10:
         TACCR0   = TAR+timers_period[timer_id];
         TACCTL0  = CCIE;
         break;
      case 11:
         TACCR1   = TAR+timers_period[timer_id];
         TACCTL1  = CCIE;
         break;
      case 12:
         TACCR2   = TAR+timers_period[timer_id];
         TACCTL2  = CCIE;
         break;
#endif
   }
}

