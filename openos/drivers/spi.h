/*
 * Driver for the SPI bus.
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef __SPI_H
#define __SPI_H

#include "openwsn.h"

// initialize SPI bus
void spi_init();
// read/write individual 8-bit registers
void spi_write_register(uint8_t reg_addr, uint8_t reg_setting);
uint8_t spi_read_register(uint8_t reg_addr);
// read/write multi-byte buffers
void spi_write_buffer(OpenQueueEntry_t* packet);
void spi_read_buffer(OpenQueueEntry_t* packet, uint8_t length);
// interrupt ISR handler
void spi_rxInterrupt();

#endif