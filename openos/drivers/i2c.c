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

#include "msp430x26x.h"
#include "i2c.h"

//=========================== variables ===========================================

unsigned char *TI_transmit_field;
unsigned char *TI_receive_field;
signed   char byteCtr;

//=========================== prototypes ==========================================

void i2c_init_transmit(unsigned char slave_address);
void i2c_transmit(unsigned char byteCount, unsigned char *field);
void i2c_init_receive(unsigned char slave_address);
void i2c_receive(unsigned char byteCount, unsigned char *field);
unsigned char i2c_busy();

//=========================== public ==============================================

void i2c_write_register(uint8_t slave_addr, uint8_t reg_addr, uint8_t reg_setting) {
   uint8_t i2c_packet[2] = {reg_addr, reg_setting};
   i2c_init_transmit(slave_addr);
   while ( i2c_busy() );
   i2c_transmit(sizeof(i2c_packet),i2c_packet);
   while ( i2c_busy() );
   __delay_cycles(I2C_BUS_FREE_TIME);
}

void i2c_read_registers(uint8_t slave_addr, uint8_t reg_addr, uint8_t numBytes, uint8_t* spaceToWrite) {
   uint8_t i2c_packet[1] = {reg_addr};
   //transmit reg address to gyro
   i2c_init_transmit(slave_addr);
   while ( i2c_busy() );
   i2c_transmit(sizeof(i2c_packet),i2c_packet);
   while ( i2c_busy() );
   __delay_cycles(I2C_BUS_FREE_TIME);
   //read what gyro has to say
   i2c_init_receive(slave_addr);
   while ( i2c_busy() );
   i2c_receive(numBytes,spaceToWrite);
   while ( i2c_busy() );
   __delay_cycles(I2C_BUS_FREE_TIME);
}

//------------------------------------------------------------------------------
// unsigned char i2c_slave_present(unsigned char slave_address)
//
// This function is used to look for a slave address on the I2C bus.  
// It sends a START and STOP condition back-to-back and wait for an ACK from the slave
//
// IN:   unsigned char slave_address  =>  Slave Address
// OUT:  unsigned char                =>  0: address was not found, 
//                                        1: address found
//------------------------------------------------------------------------------
unsigned char i2c_slave_present(unsigned char slave_address) {
   unsigned char uc1ie_bak, slaveadr_bak, ucb1i2cie_bak, returnValue;
   //store state
   ucb1i2cie_bak =  UCB1I2CIE;                   // store UCB1I2CIE register
   uc1ie_bak     =  UC1IE;                       // store UC1IE register
   slaveadr_bak  =  UCB1I2CSA;                   // store old slave address
   //start i2c
   while ( i2c_busy() );
   i2c_init_transmit(slave_address);
   //
   UCB1I2CIE    &= ~UCNACKIE;                    // no NACK interrupt
   UCB1I2CSA     =  slave_address;               // set slave address
   UC1IE        &= ~(UCB1TXIE + UCB1RXIE);       // no RX or TX interrupts
   __disable_interrupt();
   UCB1CTL1     |=  UCTR + UCTXSTT + UCTXSTP;    // transmitter, start and stop condition
   while (UCB1CTL1 & UCTXSTP);                   // wait for STOP condition
   returnValue   = !(UCB1STAT & UCNACKIFG);      // 0 if an ACK was received
   __enable_interrupt();
   
   //restore state
   UC1IE         =  uc1ie_bak;                   // restore IE2
   UCB1I2CSA     =  slaveadr_bak;                // restore old slave address
   UCB1I2CIE     =  ucb1i2cie_bak;               // restore old UCB0CTL1
   return returnValue;                           // return whether or not 
}

//=========================== helper functions ====================================

//------------------------------------------------------------------------------
// void i2c_init_transmit(unsigned char slave_address)
//
// This function initializes the USCI module for master-transmit operation. 
//
// IN:   unsigned char slave_address   =>  Slave Address
//------------------------------------------------------------------------------
void i2c_init_transmit(unsigned char slave_address){
   P5SEL     |=  SDA_PIN + SCL_PIN;              // Assign I2C pins to USCI_B1
   UCB1CTL1   =  UCSWRST;                        // set SW reset on USCI module
   UCB1CTL0   =  UCMST + UCMODE_3 + UCSYNC;      // I2C Master, synchronous mode
   UCB1CTL1   =  UCSSEL_2 + UCSWRST;             // Use SMCLK, keep SW reset
   UCB1BR0    =  I2C_PRESCALE;                   // set prescaler (SCL speed)
   UCB1BR1    =  0;
   UCB1I2CSA  =  slave_address;                  // Set slave address
   UCB1CTL1  &= ~UCSWRST;                        // clear SW reset on USCI module
   UCB1I2CIE  =  UCNACKIE;                       // Not-acknowledge interrupt enabled
   UC1IE      =  UCB1TXIE;                       // USCI_B1 transmit interrupt enabled
}

//------------------------------------------------------------------------------
// void i2c_transmit(unsigned char byteCount, unsigned char *field)
//
// This function is used to start an I2C commuincation in master-transmit mode. 
//
// IN:   unsigned char byteCount  =>  number of bytes that should be transmitted
//       unsigned char *field     =>  array variable. Its content will be sent.
//------------------------------------------------------------------------------
void i2c_transmit(unsigned char byteCount, unsigned char *field) {
   TI_transmit_field = field;
   byteCtr   = byteCount;
   UCB1CTL1 |= UCTR + UCTXSTT;                   // I2C transmitter, transmit start condition
}

//------------------------------------------------------------------------------
// void i2c_init_receive(unsigned char slave_address)
//
// This function initializes the USCI module for master-receive operation. 
//
// IN:   unsigned char slave_address   =>  Slave Address
//-----------------------------------------------------------------------------
void i2c_init_receive(unsigned char slave_address) {
   P5SEL     |=  SDA_PIN + SCL_PIN;              // Assign I2C pins to USCI_B1
   UCB1CTL1   =  UCSWRST;                        // set SW reset on USCI module
   UCB1CTL0   =  UCMST + UCMODE_3 + UCSYNC;      // I2C Master, synchronous mode
   UCB1CTL1   =  UCSSEL_2 + UCSWRST;             // Use SMCLK, keep SW reset (NOT master)
   UCB1BR0    =  I2C_PRESCALE;                   // set prescaler
   UCB1BR1    =  0;
   UCB1I2CSA  =  slave_address;                  // set slave address
   UCB1CTL1  &= ~UCSWRST;                        // clear SW reset on USCI module
   UCB1I2CIE  =  UCNACKIE;                       // No-acknowledge interrupt enabled
   UC1IE      =  UCB1RXIE;                       // Enable RX interrupt
}

//------------------------------------------------------------------------------
// void i2c_receive(unsigned char byteCount, unsigned char *field)
//
// This function is used to start an I2C commuincation in master-receiver mode. 
//
// IN:   unsigned char byteCount  =>  number of bytes that should be read
//       unsigned char *field     =>  array variable used to store received data
//------------------------------------------------------------------------------
void i2c_receive(unsigned char byteCount, unsigned char *field) {
   TI_receive_field = field;
   if ( byteCount == 1 ){
      byteCtr = 0 ;
      __disable_interrupt();
      UCB1CTL1 |= UCTXSTT;                       // I2C start condition
      while (UCB1CTL1 & UCTXSTT);                // Start condition sent?
      UCB1CTL1 |= UCTXSTP;                       // I2C stop condition
      __enable_interrupt();
   } else if ( byteCount > 1 ) {
      byteCtr = byteCount - 2 ;
      UCB1CTL1 |= UCTXSTT;                       // I2C start condition
   } else {
      while (1);                                 // illegal parameter
   }
}

//------------------------------------------------------------------------------
// unsigned char i2c_busy()
//
// This function is used to check if there is commuincation in progress. 
//
// OUT:  unsigned char  =>  0: I2C bus is idle, 
//                          1: communication is in progress
//------------------------------------------------------------------------------
unsigned char i2c_busy() {
   return (UCB1STAT & UCBBUSY);
}

//=========================== interrupt ISR handler ===========================

//executed in ISR, called from scheduler.c
void i2c_rxInterrupt() {
   // fires when a data byte is received from the slave
   if (UCB1STAT & UCNACKIFG) {                   // if slave does not send ACK (only happens after I send addr)
      UCB1CTL1 |=  UCTXSTP;                      // send STOP
      UCB1STAT &= ~UCNACKIFG;                    // clear NACK IFG
   }
}

//executed in ISR, called from scheduler.c
void i2c_txInterrupt() {
   // fires when a byte of data (including START) is written
   // from the register to the USCI shift register
   if (UC1IFG & UCB1RXIFG){                      // I received something
     *TI_receive_field = UCB1RXBUF;
      TI_receive_field++;
      if ( byteCtr == 0 ) {
         UCB1CTL1 |= UCTXSTP;                    // I2C stop condition
      } else {
         byteCtr--;
      }
   } else {
     //didn't receive anything, I will transmit
      if (byteCtr == 0){
         UCB1CTL1 |=  UCTXSTP;                   // I2C stop condition
         UC1IFG   &= ~UCB1TXIFG;                 // Clear USCI_B0 TX int flag
      } else {
         UCB1TXBUF = *TI_transmit_field;
         TI_transmit_field++;
         byteCtr--;
      }
   }
}