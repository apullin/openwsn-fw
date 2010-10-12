/*
 * Drivers for the sensitive accelerometer and temperature sensor of the GINA2.2b/c board.
 *
 * Author:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef __SENSITIVE_ACCEL_TEMPERATURE_H
#define __SENSITIVE_ACCEL_TEMPERATURE_H

#include "msp430x26x.h"
#include "stdint.h"

//prototypes
void sensitive_accel_temperature_init();
void sensitive_accel_temperature_disable();
void sensitive_accel_temperature_get_config();
void sensitive_accel_temperature_get_measurement(uint8_t* spaceToWrite);

#endif
