/*
Driver for the SPI bus.

Authors:
Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010

This driver can be used in two modes:
- When ISR_SPI is defined, the driver enables the USCI_A0 RX/TX interrupt by
  setting the corresponding bit in IE2. When it sends a byte, the driver waits
  for an interrupt to signal that the byte was sent correctly before feeding
  the USCI_A0 module with next next byte.
  Because the driver itself expects interrupts, you can not call any of these
  functions in interrupt mode, since you can not get interrupted in that mode.
  If you're very concerned about timing (e.g. you're using 15.4e), I recommend
  you use the second mode.
- When ISR_SPI is *not* defined, this driver does not use interrupts. Instead,
  after putting a byte in the TX buffer, the driver busy waits (i.e. the CPU
  is active) while waiting for the UCA0RXIFG bit in IFG2 to set. Once that
  happens, the driver manually clears that bit and writes the next byte in the
  TX buffer.
  While this means the CPU is active throughout the transmission and can not do
  anything else, it also means the execution is perfectly deterministic if the 
  initial function is called from ISR. Hence the recommended use of that mode
  when using IEEE802.15.4e.
*/

#include "msp430x26x.h"
#include "spi.h"
#include "radio.h"
#include "packetfunctions.h"

//===================================== variables =============================

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

//===================================== prototypes ============================

void spi_txrx(uint8_t* spaceToSend, uint8_t len, uint8_t* spaceToReceive);

//===================================== public ================================

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
#ifdef ISR_SPI
   IE2      |=  UCA0RXIE;                        // Enable USCI_A0 RX/TX interrupt (TX and RX happen at the same time)
#endif
}

void spi_write_register(uint8_t reg_addr, uint8_t reg_setting) {
   uint8_t temp_tx_buffer[2];
   uint8_t temp_rx_buffer[2];
   temp_tx_buffer[0] = (0xC0 | reg_addr);        // turn addess in a 'reg write' address
   temp_tx_buffer[1] = reg_setting;
   spi_txrx(&(temp_tx_buffer[0]),sizeof(temp_tx_buffer),&(temp_rx_buffer[0]));
#ifdef ISR_SPI
   while ( spi_busy==1 );
#endif
}

uint8_t spi_read_register(uint8_t reg_addr) {
   uint8_t temp_tx_buffer[2];
   uint8_t temp_rx_buffer[2];
   temp_tx_buffer[0] = (0x80 | reg_addr);        // turn addess in a 'reg read' address
   temp_tx_buffer[1] = 0x00;                     // send a no_operation command just to get the reg value
   spi_txrx(&(temp_tx_buffer[0]),sizeof(temp_tx_buffer),&(temp_rx_buffer[0]));
#ifdef ISR_SPI
   while ( spi_busy==1 );
#endif
   return temp_rx_buffer[1];
}

void spi_write_buffer(OpenQueueEntry_t* packet) {
   uint8_t temp_rx_buffer[1+1+127];              // 1B SPI address, 1B length, max. 127B data
   spi_txrx(packet->payload,packet->length,&(temp_rx_buffer[0]));
#ifdef ISR_SPI
   while ( spi_busy==1 );
#endif
}

void spi_read_buffer(OpenQueueEntry_t* packet, uint8_t length) {
   uint8_t temp_tx_buffer[1+1+127];              // 1B SPI address, 1B length, 127B data
   temp_tx_buffer[0] = 0x20;                     // spi address for 'read frame buffer'
   spi_txrx(&(temp_tx_buffer[0]),length,packet->payload);
#ifdef ISR_SPI
   while ( spi_busy==1 );
#endif
}

//===================================== private ===============================

#ifdef ISR_SPI
// this implemetation uses interrupts to signal that a byte was sent
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
#else
// this implemetation busy waits for each byte to be sent
void spi_txrx(uint8_t* spaceToSend, uint8_t len, uint8_t* spaceToReceive) {
   //register spi frame to send
   spi_tx_buffer = spaceToSend;
   spi_rx_buffer = spaceToReceive;
   num_bytes     = len;
   // SPI CS (and P4.6) down
   P4OUT&=~0x01;P4OUT&=~0x40;                    // SPI CS (and P4.6) down
   // write all bytes
   while (num_bytes>0) {
      //write byte to TX buffer
      UCA0TXBUF     = *spi_tx_buffer;
      spi_tx_buffer++;
      // busy wait on the interrupt flag
      while ((IFG2 & UCA0RXIFG)==0);
      // clear the interrupt flag
      IFG2 &= ~UCA0RXIFG;
      // save the byte just received in the RX buffer
      *spi_rx_buffer = UCA0RXBUF;
      spi_rx_buffer++;
      // one byte less to go
      num_bytes--;
   }
   // SPI CS (and P4.6) up
   P4OUT|=0x01;P4OUT|=0x40;
}
#endif

//=========================== interrupt ISR handler ===========================

#ifdef ISR_SPI
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
#endif
