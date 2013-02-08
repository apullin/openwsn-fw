/**
\brief ImageProc2.4 specific definition of the "debugpins" bsp module.

\author Andrew Pullin <pullin@berkeley.edu>, January 2013.
 */

#include "p33fj128mc706a.h"

#include "debugpins.h"

//=========================== defines =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

//=========================== public ==========================================

void debugpins_init() {
    //Debug pins on camera connector
    TRISFbits.TRISF1 = 0;
    TRISDbits.TRISD2 = 0;
    TRISDbits.TRISD1 = 0;
    TRISDbits.TRISD3 = 0;
    TRISDbits.TRISD0 = 0;
    TRISDbits.TRISD4 = 0;

    debugpins_frame_set();
    debugpins_fsm_set();
    Nop();
    Nop();
    debugpins_frame_clr();
    debugpins_fsm_clr();

}

// RD2
void debugpins_frame_toggle() {
    _RD2 ^= 0x1;
}

void debugpins_frame_clr() {
    _RD2 = 0x0;
}

void debugpins_frame_set() {
    _RD2 = 0x1;
}

// RD1
void debugpins_slot_toggle() {
    _RD1 ^= 0x1;
}

void debugpins_slot_clr() {
    _RD1 = 0x0;
}

void debugpins_slot_set() {
    _RD1 = 0x1;
}

// RD3
void debugpins_fsm_toggle() {
    _RD3 ^= 0x1;
}

void debugpins_fsm_clr() {
    _RD3 = 0x0;
}

void debugpins_fsm_set() {
    _RD3 = 0x1;
}

// RD0
void debugpins_task_toggle() {
    _RD0 ^= 0x1;
}

void debugpins_task_clr() {
    _RD0 = 0x0;
}

void debugpins_task_set() {
    _RD0 = 0x1;
}

// RD4
void debugpins_isr_toggle() {
    _RD4 ^= 0x1;
}

void debugpins_isr_clr() {
    _RD4 = 0x0;
}

void debugpins_isr_set() {
    _RD4 = 0x1;
}

// RF1
void debugpins_radio_toggle() {
    _RF1 ^= 0x1;
}

void debugpins_radio_clr() {
    _RF1 = 0x0;
}

void debugpins_radio_set() {
    _RF1 = 0x1;
}

//=========================== private =========================================