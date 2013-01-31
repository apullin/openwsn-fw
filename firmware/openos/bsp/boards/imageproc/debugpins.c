/**
\brief ImageProc2.4-specific definition of the "debugpins" bsp module.

\author Andrew Pullin <pullin@berkeley.edu>, January 2013.
*/

#include "p33fj128mc706a.h"

#include "debugpins.h"

//=========================== defines =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

//=========================== public ==========================================

void debugpins_init() {
    //TODO: find usable GPIO for this on ImageProc2.4
   //P6DIR |=  0x40;      // frame [P6.6]
   //P6DIR |=  0x80;      // slot  [P6.7]
   //P2DIR |=  0x08;      // fsm   [P2.3]
   //P2DIR |=  0x40;      // task  [P2.6]
   //P6DIR |=  0x01;      // isr   [P6.0]
   //P6DIR |=  0x02;      // radio [P6.1]
}

// P6.6
void debugpins_frame_toggle() {
   //P6OUT ^=  0x40;
}
void debugpins_frame_clr() {
   //P6OUT &= ~0x40;
}
void debugpins_frame_set() {
   //P6OUT |=  0x40;
}

// P6.7
void debugpins_slot_toggle() {
   //P6OUT ^=  0x80;
}
void debugpins_slot_clr() {
   //P6OUT &= ~0x80;
}
void debugpins_slot_set() {
   //P6OUT |=  0x80;
}

// P2.3
void debugpins_fsm_toggle() {
   //P2OUT ^=  0x08;
}
void debugpins_fsm_clr() {
   //P2OUT &= ~0x08;
}
void debugpins_fsm_set() {
   //P2OUT |=  0x08;
}

// P2.6
void debugpins_task_toggle() {
   //P2OUT ^=  0x40;
}
void debugpins_task_clr() {
   //P2OUT &= ~0x40;
}
void debugpins_task_set() {
   //P2OUT |=  0x40;
}

// P6.0
void debugpins_isr_toggle() {
   //P6OUT ^=  0x01;
}
void debugpins_isr_clr() {
   //P6OUT &= ~0x01;
}
void debugpins_isr_set() {
   //P6OUT |=  0x01;
}

// P6.1
void debugpins_radio_toggle() {
   //P6OUT ^=  0x02;
}
void debugpins_radio_clr() {
   //P6OUT &= ~0x02;
}
void debugpins_radio_set() {
   //P6OUT |=  0x02;
}

//=========================== private =========================================