/**
\brief Driver for the Leds of the GINA2.2b/c boards..

\author Ankur Mehta <mehtank@eecs.berkeley.edu>, August 2010
\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/

#ifndef __LEDS_H
#define __LEDS_H

#include "msp430x26x.h"
#include "stdint.h"
 
//=========================== define ==========================================

#define LED0_ON()       P2OUT |=  0x01;
#define LED0_OFF()      P2OUT &= ~0x01;
#define LED0_TOGGLE()   P2OUT ^=  0x01;

#define LED1_ON()       P2OUT |=  0x02;
#define LED1_OFF()      P2OUT &= ~0x02;
#define LED1_TOGGLE()   P2OUT ^=  0x02;

#define LED2_ON()       P2OUT |=  0x04;
#define LED2_OFF()      P2OUT &= ~0x04;
#define LED2_TOGGLE()   P2OUT ^=  0x04;

#define LED3_ON()       P2OUT |=  0x08;
#define LED3_OFF()      P2OUT &= ~0x08;
#define LED3_TOGGLE()   P2OUT ^=  0x08;

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void leds_init();
void leds_circular_shift();
void leds_increment();

#endif
