/*
 * Driver for the Leds of the GINA2.2b/c boards..
 *
 * Authors:
 * Ankur Mehta <mehtank@eecs.berkeley.edu>, August 2010
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef __LEDS_H
#define __LEDS_H

#include "msp430x26x.h"
#include "stdint.h"
 
//prototypes
void leds_init();
void leds_circular_shift();
void leds_increment();

#endif
