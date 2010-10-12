/*
 * Driver for the I2C bus, initialy downloaded from Texas Instruments,
 * heavily modified since.
 *
 * Authors:
 * Uli Kretzschmar, Texas Instruments Deutschland GmbH, November 2007
 * Hoam Chung, June 2010
 * Leo Keselman <lkeselman@berkeley.edu>, July 2010
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef ___I2C_H
#define ___I2C_H

#include "stdint.h"

#define I2C_PRESCALE        0x26                 // I2C SCL speed (->392kHz @ 16MHz)
#define I2C_BUS_FREE_TIME   100                  // 100 ticks @8MHz ~ 12us
#define SDA_PIN             0x02                 // msp430x261x UCB1SDA pin
#define SCL_PIN             0x04                 // msp430x261x UCB1SCL pin

void i2c_read_registers(uint8_t slave_addr,
                             uint8_t reg_addr,
                             uint8_t numBytes,
                             uint8_t* spaceToWrite);
void i2c_write_register(uint8_t slave_addr,
                             uint8_t reg_addr,
                             uint8_t reg_setting);
unsigned char i2c_slave_present(unsigned char slave_address);
// interrupt ISR handlers
void i2c_txInterrupt();
void i2c_rxInterrupt();

#endif
