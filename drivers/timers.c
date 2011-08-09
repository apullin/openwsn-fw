/*
 * Driver for the general purpose timers.
 *
 * Authors:
 * Ankur Mehta <watteyne@eecs.berkeley.edu>, October 2010
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 *
 * This file drives all timers off of TimerB clocked from the 32kHz crystal, in
 * continuous mode.
 *
 * It's a really simple implementation, in which each module which needs a timer
 * is assigned one of the 7 TimerB compare registers. This means that at most 7
 * modules can be assigned a timer.
 * 
 * When we'll need more, we'll have implement some software fanciness which will
 * run all timer off of a single compare register, offer dynamic timer creation,
 * etc, etc.
 *
 * For now, KISS.
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
   
   BCSCTL3 |= LFXT1S_0;                          // source ACLK from 32kHz crystal

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
         TBCCR0   =  0;
         TBCCTL0 &= ~CCIE;
         break;
      case 1:
         TBCCR1   =  0;
         TBCCTL1 &= ~CCIE;
         break;
      case 2:
         TBCCR2   =  0;
         TBCCTL2 &= ~CCIE;
         break;
      case 3:
         TBCCR3   =  0;
         TBCCTL3 &= ~CCIE;
         break;
      case 4:
         TBCCR4   =  0;
         TBCCTL4 &= ~CCIE;
         break;
      case 5:
         TBCCR5   =  0;
         TBCCTL5 &= ~CCIE;
         break;
      case 6:
         TBCCR6   =  0;
         TBCCTL6 &= ~CCIE;
         break;
   }
}

//=========================== private =============================================

void timer_start(uint8_t timer_id, uint16_t duration, bool continuous) {
   timers_period[timer_id]     = duration;
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
   }
}

