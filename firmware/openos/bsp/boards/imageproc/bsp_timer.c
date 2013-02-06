/**
\brief ImageProc2.4-specific definition of the "bsp_timer" bsp module.

On IP2.4, we use Timer3 for the bsp_timer module.
 This is potentially an unresolved problem; Timer1 does not have output compare,
 but Timer1 is the only timer which can be run from a 32Khz crystal.
 TODO (apullin) : Can bsp_timer be rewritten to use a timer module without OC interrupts?

\author Andrew Pullin <pullin@berkeley.edu>, January 2013.
*/

#include "p33fj128mc706a.h"
#include "string.h"
#include "bsp_timer.h"
#include "board.h"
#include "board_info.h"
#include "timer.h"

//=========================== defines =========================================

//=========================== variables =======================================

typedef struct {
   bsp_timer_cbt    cb;
   PORT_TIMER_WIDTH last_compare_value;
} bsp_timer_vars_t;

bsp_timer_vars_t bsp_timer_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

/**
\brief Initialize this module.

This functions starts the timer, i.e. the counter increments, but doesn't set
any compare registers, so no interrupt will fire.
*/
void bsp_timer_init() {
   
   // clear local variables
   memset(&bsp_timer_vars,0,sizeof(bsp_timer_vars_t));
   
   DisableIntT3;
    T3CON = T3_OFF          //Start timer off
            & T3_IDLE_CON   //Timer 2 continues in Idle() mode
            & T3_GATE_OFF   //Gated timer mode off
            & T3_PS_1_64    //1:64 prescaler, so max period = 104.856 ms
            & T3_SOURCE_INT; //Internal, Fosc / 2

   //set CCRB0 registers
   //TBCCTL0              =  0;
   OC3R = 0;
   OC3CON = 0; //TODO : how to configure this?
   //TBCCR0               =  0;

   // start Timer3
   EnableIntT3;    // Enable T2 interrupt, when counter resets
   T3CONbits.TON = 1; // Start T2 running
   //TODO : set interrupt priority ?
}

/**
\brief Register a callback.

\param cb The function to be called when a compare event happens.
*/
void bsp_timer_set_callback(bsp_timer_cbt cb) {
   bsp_timer_vars.cb   = cb;
}

/**
\brief Reset the timer.

This function does not stop the timer, it rather resets the value of the
counter, and cancels a possible pending compare event.
*/
void bsp_timer_reset() {
   // reset compare
   //TBCCR0               =  0;
   OC3R = 0;
   //TBCCTL0              =  0;
   OC3CON = 0; //TODO : configure properly
   // reset timer
   TMR3                  = 0;
   // record last timer compare value
   bsp_timer_vars.last_compare_value =  0;
}

/**
\brief Schedule the callback to be called in some specified time.

The delay is expressed relative to the last compare event. It doesn't matter
how long it took to call this function after the last compare, the timer will
expire precisely delayTicks after the last one.

The only possible problem is that it took so long to call this function that
the delay specified is shorter than the time already elapsed since the last
compare. In that case, this function triggers the interrupt to fire right away.

This means that the interrupt may fire a bit off, but this inaccuracy does not
propagate to subsequent timers.

\param delayTicks Number of ticks before the timer expired, relative to the
                  last compare event.
*/
void bsp_timer_scheduleIn(PORT_TIMER_WIDTH delayTicks) {
   PORT_TIMER_WIDTH newCompareValue;
   PORT_TIMER_WIDTH temp_last_compare_value;
   
   temp_last_compare_value = bsp_timer_vars.last_compare_value;
   
   newCompareValue      =  bsp_timer_vars.last_compare_value+delayTicks+1;
   bsp_timer_vars.last_compare_value   =  newCompareValue;
   
   if (delayTicks < TMR3 - temp_last_compare_value) {
      // we're already too late, schedule the ISR right now manually
      
      // setting the interrupt flag triggers an interrupt
      _T3IF         =  1;
   } else {
      // this is the normal case, have timer expire at newCompareValue
      OC3R            =  newCompareValue;
      _OC3IE         = 1;
   }
}

/**
\brief Cancel a running compare.
*/
void bsp_timer_cancel_schedule() {
   //TBCCR0               =  0;
   OC3R = 0;
   //TBCCTL0             &= ~CCIE;
   _OC3IE = 0;
}

/**
\brief Return the current value of the timer's counter.

\returns The current value of the timer's counter.
*/
PORT_TIMER_WIDTH bsp_timer_get_currentValue() {
   return TMR3;
}

//=========================== private =========================================

//=========================== interrupt handlers ==============================

kick_scheduler_t bsp_timer_isr() {
   // call the callback
   bsp_timer_vars.cb();
   // kick the OS
   return KICK_SCHEDULER;
}