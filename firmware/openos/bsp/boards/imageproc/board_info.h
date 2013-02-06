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
#define PORT_TICS_PER_MS                    625 //40Mhz/64 = 1.6 us per tick, 1.6us*625=1ms , AP

// on IP2.4, we use the CAN1 interrupt for the OS
#define SCHEDULER_WAKEUP()                  C1INTFbits.RBIF = 1; //Initiate ECAN1 RX interrupt, as a fake
#define SCHEDULER_ENABLE_INTERRUPT()        C1INTEbits.RBIE = 1; //Enable ECAN1 RX interrupt, as a fake



//===== pinout

//radio VREG
#define PORT_PIN_RADIO_SLP_TR_CNTL_HIGH()           _LATB15 = 1;
#define PORT_PIN_RADIO_SLP_TR_CNTL_LOW()            _LATB15 = 0;
//radio RESET
#define PORT_PIN_RADIO_RESET_HIGH()         //nothing, not connected on IP2.4
#define PORT_PIN_RADIO_RESET_LOW()          //nothing, not connected on IP2.4

//===== IEEE802154E timing

// time-slot related
#define PORT_TsSlotDuration                 9299   // AP
// execution speed related
#define PORT_maxTxDataPrepare               359    //AP   // 2014us (measured 746us)
#define PORT_maxRxAckPrepare                190    //AP   //  305us (measured  83us)
#define PORT_maxRxDataPrepare               625    //AP   // 1007us (measured  84us)
#define PORT_maxTxAckPrepare                417    //AP   //  305us (measured 219us)
// radio speed related
#define PORT_delayTx                        21     //AP   //  214us (measured 219us)
#define PORT_delayRx                        0     //AP   //    0us (can not measure)
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
