/**
\brief ImageProc2.4-specific definition of the "radiotimer" bsp module.

On IP2.4, we use Timer2 for the radio_timer module.
 This is potentially an unresolved problem; Timer1 does not have output compare,
 but Timer1 is the only timer which can be run from a 32Khz crystal.
 TODO (apullin) : Can bsp_timer be rewritten to use a timer module without OC interrupts?

\author Andrew Pullin <pullin@berkeley.edu>, January 2013.
*/

#include "p33fj128mc706a.h"
#include "stdio.h"
#include "string.h"
#include "radiotimer.h"
#include "timer.h" //from Microchip library
#include "outcompare.h"
#include "incap.h"

//=========================== variables =======================================

typedef struct {
   radiotimer_compare_cbt    overflow_cb;
   radiotimer_compare_cbt    compare_cb;
} radiotimer_vars_t;

radiotimer_vars_t radiotimer_vars;

//=========================== prototypes ======================================
kick_scheduler_t radiotimer_overflow_isr();
kick_scheduler_t radiotimer_compare_isr();

//=========================== public ==========================================

//===== admin

void radiotimer_init() {
   // clear local variables
   memset(&radiotimer_vars,0,sizeof(radiotimer_vars_t));
}

void radiotimer_setOverflowCb(radiotimer_compare_cbt cb) {
   radiotimer_vars.overflow_cb    = cb;
}

void radiotimer_setCompareCb(radiotimer_compare_cbt cb) {
   radiotimer_vars.compare_cb     = cb;
}

void radiotimer_setStartFrameCb(radiotimer_capture_cbt cb) {
   while(1);
}

void radiotimer_setEndFrameCb(radiotimer_capture_cbt cb) {
   while(1);
}

void radiotimer_start(uint16_t period) {

    DisableIntT2;
    T2CON = T2_OFF          //Start timer off
            & T2_IDLE_CON   //Timer 2 continues in Idle() mode
            & T2_GATE_OFF   //Gated timer mode off
            & T2_PS_1_64    //1:64 prescaler, so max period = 104.856 ms
            & T2_32BIT_MODE_OFF //Timer runs in single 16 bit timer mode
            & T2_SOURCE_INT; //Internal, Fosc / 2
   
   //Overall period of the timer; _T2Interrupt is called when TMR2 == PR2
    PR2 = period;
   
   // OC1 in compare mode (disabled for now)
   OC1CON  =  OC_IDLE_CON & OC_PWM_FAULT_PIN_DISABLE & OC_TIMER2_SRC &
           OC_OFF;
   OC1R   =  0;
   _OC1IF = 0; // Clear int flag
   _OC1IE = 0; // Interrupt disabled at start
   
   // reset Timer2 couter
   TMR2 = 0;
   
   // start counting
   EnableIntT2;    // Enable T2 interrupt, when counter resets                       
   T2CONbits.TON = 1; // Start T2 running
   //TODO : set interrupt priority ?
}

//===== direct access

uint16_t radiotimer_getValue() {
   return TMR2;
}

void radiotimer_setPeriod(uint16_t period) {
   PR2   =  period; //Overflow period for Timer2
}

uint16_t radiotimer_getPeriod() {
   return PR2; //Overflow period for Timer2
}

//===== compare

void radiotimer_schedule(uint16_t offset) {
   
   OC1R   =  offset; // offset when to fire
   _OC1IF = 0;      //Clear flag
   EnableIntOC1; // enable OC1 interrupt
   OC1CONbits.OCM = 0b001; // Acitve-low one shot mode; pin output irrelevant
}

void radiotimer_cancel() {
    OC1CONbits.OCM = 0b000;  // OC1 module OFF
  OC1R = 0; // Reset OC2 value, and clear interrupt flag
  _OC1IF = 0;
  DisableIntOC1; // disable OC2 interrupt
}

//===== capture

inline uint16_t radiotimer_getCapturedTime() {
   return TMR2;
}

//=========================== private =========================================

//=========================== interrupt handlers ==============================

kick_scheduler_t radiotimer_overflow_isr() {

    if (radiotimer_vars.overflow_cb != NULL) {
        // call the callback
        radiotimer_vars.overflow_cb();
        // kick the OS
        return KICK_SCHEDULER;
    }

    return DO_NOT_KICK_SCHEDULER;
}

kick_scheduler_t radiotimer_compare_isr() {
    // capture/compare OC1
    if (radiotimer_vars.compare_cb != NULL) {
        // call the callback
        radiotimer_vars.compare_cb();
        // kick the OS
        return KICK_SCHEDULER;
    }

    return DO_NOT_KICK_SCHEDULER;
}