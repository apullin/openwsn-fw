/*
 * Driver for the SPI bus.
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#include "msp430x26x.h"
#include "spi.h"
#include "radio.h"
#include "packetfunctions.h"

//=========================== variables ===========================================

uint8_t* spi_tx_buffer;
uint8_t* spi_rx_buffer;
uint8_t  num_bytes;
uint8_t  spi_busy;

struct {
   volatile unsigned char* ctl0[2];
   volatile unsigned char* ctl1[2];
   volatile unsigned char* br0[2];
   volatile unsigned char* br1[2];
   volatile unsigned char* ie[2];
   volatile unsigned char* txbuf[2];
   volatile unsigned char* rxbuf[2];
} spi_control;

/*
 * CTL0
 * CTL1
 * BR0
 * BR1
 * P4PINS
 * IE2
 * TXBUF
 * RXBUF
 */
//=========================== prototypes ==========================================

void spi_txrx(uint8_t* spaceToSend, uint8_t len, uint8_t* spaceToReceive);

//=========================== public ==============================================

void spi_init() {
   UCA0CTL1  =  UCSSEL1 + UCSSEL0 + UCSWRST;     // SMCLK, reset
   UCA0CTL0  =  UCCKPH + UCMSB + UCMST + UCSYNC; // polarity, MSB first, master mode, 3-pin SPI
   UCA0BR0   =  0x03;                            // UCLK/4
   UCA0BR1   =  0x00;                            // 0
   P3SEL    |=  0x31;                            // P3SEL = 0bxx11xxx1, MOSI/MISO/CLK pins
   P4OUT    |=  0x01;                            // EN_RF (P4.0) pin is SPI chip select, set high
   P4DIR    |=  0x01;                            // EN_RF (P4.0) pin as output
   P4OUT    |=  0x40;                            // set P4.6 pin high (mimicks EN_RF)
   P4DIR    |=  0x40;                            // P4.6 pin as output
   UCA0CTL1 &= ~UCSWRST;                         // Initialize USART state machine
   IE2      |=  UCA0RXIE;                        // Enable USCI_A0 RX/TX interrupt
}

void spi_write_register(uint8_t reg_addr, uint8_t reg_setting) {
   uint8_t temp_tx_buffer[2];
   uint8_t temp_rx_buffer[2];
   temp_tx_buffer[0] = (0xC0 | reg_addr);        // turn addess in a 'reg write' address
   temp_tx_buffer[1] = reg_setting;
   spi_txrx(&(temp_tx_buffer[0]),sizeof(temp_tx_buffer),&(temp_rx_buffer[0]));
   while ( spi_busy==1 );
}

uint8_t spi_read_register(uint8_t reg_addr) {
   uint8_t temp_tx_buffer[2];
   uint8_t temp_rx_buffer[2];
   temp_tx_buffer[0] = (0x80 | reg_addr);        // turn addess in a 'reg read' address
   temp_tx_buffer[1] = 0x00;                     // send a no_operation command just to get the reg value
   spi_txrx(&(temp_tx_buffer[0]),sizeof(temp_tx_buffer),&(temp_rx_buffer[0]));
   while ( spi_busy==1 );
   return temp_rx_buffer[1];
}

void spi_write_buffer(OpenQueueEntry_t* packet) {
   uint8_t temp_rx_buffer[1+1+127];              // 1B SPI address, 1B length, max. 127B data
   spi_txrx(packet->payload,packet->length,&(temp_rx_buffer[0]));
   while ( spi_busy==1 );
}

void spi_read_buffer(OpenQueueEntry_t* packet, uint8_t length) {
   uint8_t temp_tx_buffer[1+1+127];              // 1B SPI address, 1B length, 127B data
   temp_tx_buffer[0] = 0x20;                     // spi address for 'read frame buffer'
   spi_txrx(&(temp_tx_buffer[0]),length,packet->payload);
   while ( spi_busy==1 );
}

//=========================== private =============================================

void spi_txrx(uint8_t* spaceToSend, uint8_t len, uint8_t* spaceToReceive) {
   __disable_interrupt();
   //register spi frame to send
   spi_tx_buffer =  spaceToSend;
   spi_rx_buffer =  spaceToReceive;
   num_bytes     =  len;
   spi_busy      =  1;
   //send first byte
   P4OUT&=~0x01;P4OUT&=~0x40;                    // SPI CS (and P4.6) down
   UCA0TXBUF     = *spi_tx_buffer;
   spi_tx_buffer++;
   num_bytes--;
   __enable_interrupt();
}

//=========================== interrupt ISR handler ===========================

//executed in ISR, called from scheduler.c
void spi_rxInterrupt() {
   *spi_rx_buffer = UCA0RXBUF;
   spi_rx_buffer++;
   if (num_bytes>0) {
      UCA0TXBUF = *spi_tx_buffer;
      spi_tx_buffer++;
      num_bytes--;
   } else {
      P4OUT|=0x01;P4OUT|=0x40;                   // SPI CS (and P4.6) up
      spi_busy = 0;
   }
}
