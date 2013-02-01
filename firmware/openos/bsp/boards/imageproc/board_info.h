/**
\brief ImageProc2.4-specific board information bsp module.

This module simply defines some strings describing the board, which CoAP uses
to return the board's description.

\author Andrew Pullin <pullin@berkeley.edu>, Jannuary 2013.
*/

#ifndef __BOARD_INFO_H
#define __BOARD_INFO_H

#include "stdint.h"
#include "p33fj128mc706a.h"
#include "string.h"
#include "Generic.h"

//=========================== define ==========================================
//processor scpecific

#define port_INLINE                         inline

#define PRAGMA(x)  
#define PACK(x)    __pack


#define INTERRUPT_DECLARATION() char saved_ipl;
#define DISABLE_INTERRUPTS()    SET_AND_SAVE_CPU_IPL(saved_ipl, 7);
#define ENABLE_INTERRUPTS()     RESTORE_CPU_IPL(saved_ipl);

// MSP430 implementaiton retained here for reference: 
//#define INTERRUPT_DECLARATION() __istate_t s;
//#define DISABLE_INTERRUPTS()    s = __get_interrupt_state(); __disable_interrupt();
//#define ENABLE_INTERRUPTS()     __set_interrupt_state(s);

//===== timer

#define PORT_TIMER_WIDTH                    uint16_t
#define PORT_SIGNED_INT_WIDTH               int16_t
#define PORT_TICS_PER_MS                    33

// on TelosB, we use the comparatorA interrupt for the OS
#define SCHEDULER_WAKEUP()                  CACTL1 |= CAIFG
#define SCHEDULER_ENABLE_INTERRUPT()        CACTL1  = CAIE

//===== pinout

// [P4.5] radio VREG
#define PORT_PIN_RADIO_VREG_HIGH()          P4OUT |=  0x20;
#define PORT_PIN_RADIO_VREG_LOW()           P4OUT &= ~0x20;
// [P4.6] radio RESET
#define PORT_PIN_RADIO_RESET_HIGH()         P4OUT |=  0x40;
#define PORT_PIN_RADIO_RESET_LOW()          P4OUT &= ~0x40;  

//===== IEEE802154E timing

// time-slot related
#define PORT_TsSlotDuration                 491   // counter counts one extra count, see datasheet
// execution speed related
#define PORT_maxTxDataPrepare               100    //  2899us (measured 2420us)
#define PORT_maxRxAckPrepare                20    //   610us (measured  474us)
#define PORT_maxRxDataPrepare               33    //  1000us (measured  477us)
#define PORT_maxTxAckPrepare                40    //   792us (measured  746us)- cannot be bigger than 28.. is the limit for telosb as actvitiy_rt5 is executed almost there.
// radio speed related
#define PORT_delayTx                        12    //   366us (measured  352us)
#define PORT_delayRx                        0     //     0us (can not measure)
// radio watchdog

//=========================== variables =======================================

static const uint8_t rreg_uriquery[]        = "h=ucb";
static const uint8_t infoBoardname[]        = "ImageProc2.4";
static const uint8_t infouCName[]           = "dsPIC33FJ128MC706A";
static const uint8_t infoRadioName[]        = "AT86RF233";

//=========================== prototypes ======================================

//=========================== public ==========================================

//=========================== private =========================================

#endif
