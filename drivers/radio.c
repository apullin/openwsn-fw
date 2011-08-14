/**
\brief Radio driver for the Atmel AT86RF231 IEEE802.15.4-compliant radio

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
*/

#include "gina_config.h"
#include "radio.h"
#include "packetfunctions.h"
#include "openqueue.h"
#include "IEEE802154E.h"
#include "ieee154etimer.h"
#include "spi.h"
#include "openserial.h"

//=========================== variables =======================================

typedef struct {
   uint8_t state;  // the current state of the radio (only used for debug)
} radio_vars_t;

radio_vars_t radio_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

/**
\brief This function initializes this module.

Call this function once before any other function in this module, possibly
during boot-up.
*/
void radio_init() {
   // change state
   radio_vars.state = RADIOSTATE_STOPPED;
   
   // set the radio debug pin as output
   DEBUG_PIN_RADIO_INIT();
   
   // initialize communication between MSP430 and radio
   //-- 4-wire SPI
   spi_init();
   //-- RF_SLP_TR_CNTL (P4.7) pin (output)
   P4OUT  &= ~0x80;                              // set low
   P4DIR  |=  0x80;                              // configure as output
   //-- IRQ_RF (P1.6) pin (input)
   P1OUT  &= ~0x40;                              // set low
   P1DIR  &= ~0x40;                              // input direction
   P1IES  &= ~0x40;                              // interrup when transition is low-to-high
   P1IE   |=  0x40;                              // enable interrupt
   
   // configure the radio
   spi_write_register(RG_TRX_STATE, CMD_FORCE_TRX_OFF);  // turn radio off
   spi_write_register(RG_IRQ_MASK, 0x0C);                // tell radio to fire interrupt on TRX_END and RX_START
   spi_read_register(RG_IRQ_STATUS);                     // deassert the interrupt pin (P1.6) in case is high
   spi_write_register(RG_ANT_DIV, RADIO_CHIP_ANTENNA);// use chip antenna
#define RG_TRX_CTRL_1 0x04
   spi_write_register(RG_TRX_CTRL_1, 0x20);              // have the radio calculate CRC   

   //busy wait until radio status is TRX_OFF
   while((spi_read_register(RG_TRX_STATUS) & 0x1F) != TRX_OFF);
   
   // change state
   radio_vars.state = RADIOSTATE_RFOFF;
}

//======= sending a packet

/**
\brief Set the radio frequency.

This function will write the frequency register in the radio over
SPI.

\param [in] frequency The frequency to set the radio at, an
                      integer between 11 (2.405GHz) and
                      26 (2.480GHz).
*/
void radio_setFrequency(uint8_t frequency) {
   // change state
   radio_vars.state = RADIOSTATE_SETTING_FREQUENCY;
   
   // make sure the frequency asked for is within bounds
   if (frequency < 11 || frequency > 26){
      frequency = 26;
   }
   
   // configure the radio to the right frequecy
   spi_write_register(RG_PHY_CC_CCA,0x20+frequency);
   
   // change state
   radio_vars.state = RADIOSTATE_FREQUENCY_SET;
}

/**
\brief Load a packet in the radio's TX buffer.

\param [in] packet The packet to write into the buffer.
*/
void radio_loadPacket(OpenQueueEntry_t* packet) {
   // change state
   radio_vars.state = RADIOSTATE_LOADING_PACKET;
   
   // don't declare radio as owner or else MAC will not be able to retransmit
   
   // add 1B length at the beginning (PHY header)
   packetfunctions_reserveHeaderSize(packet,1);
   packet->payload[0] = packet->length-1;   // length (not counting length field)
   
   // add 1B SPI address at the beginning (internally for SPI)
   packetfunctions_reserveHeaderSize(packet,1);
   packet->payload[0] = 0x60;
   
   // load packet in TXFIFO
   spi_write_buffer(packet);
   
   // remove the 2 header bytes just added so MAC layer doesn't get confused
   // when retransmitting
   packetfunctions_tossHeader(packet,2);
   
   // change state
   radio_vars.state = RADIOSTATE_PACKET_LOADED;
}

/**
\brief Enable the radio in TX mode.

This function turns everything on in the radio so it can
transmit, but does not actually transmit the bytes.
*/
void radio_txEnable() {
   // change state
   radio_vars.state = RADIOSTATE_ENABLING_TX;
   
   // wiggle debug pin
   DEBUG_PIN_RADIO_SET();
   
   // turn on radio's PLL
   spi_write_register(RG_TRX_STATE, CMD_PLL_ON);
   while((spi_read_register(RG_TRX_STATUS) & 0x1F) != PLL_ON); // busy wait until done
   
   // change state
   radio_vars.state = RADIOSTATE_TX_ENABLED;
}

/**
\brief Start transmitting.

Tells the radio to transmit the packet immediately.
*/
void radio_txNow() {
   // change state
   radio_vars.state = RADIOSTATE_TRANSMITTING;
   
   // send packet by pulsing the RF_SLP_TR_CNTL pin
   P4OUT |=  0x80;
   P4OUT &= ~0x80;
   
   // The AT86RF231 does not generate an interrupt when the radio transmits the
   // SFD. If we leave this funtion like this, tt3 will expire, triggering tie2.
   // Instead, we will cheat an mimick a start of frame event by calling
   // ieee154e_startOfFrame from here
   ieee154e_startOfFrame(ieee154etimer_getCapturedTime());
}

//======= receiving a packet

/**
\brief Enable the radio in RX mode.

This function turns everything on in the radio so it can
receive, but does not actually receive the bytes.
*/
void radio_rxEnable() {
   // change state
   radio_vars.state = RADIOSTATE_ENABLING_RX;
   
   // put radio in reception mode
   spi_write_register(RG_TRX_STATE, CMD_RX_ON);
   
   // wiggle debug pin
   DEBUG_PIN_RADIO_SET();
   
   //busy wait until radio status is PLL_ON
   while((spi_read_register(RG_TRX_STATUS) & 0x1F) != RX_ON);
   
   // change state
   radio_vars.state = RADIOSTATE_LISTENING;
}

/**
\brief Start receoving.

Tells the radio to starte listening for a packet immediately.
*/
void radio_rxNow() {
   // this function doesn't do anything since this radio is already
   // listening after radio_rxEnable() is called.
}

/**
\brief Retrieves the received frame from the radio's RX buffer.

Copies the bytes of the packet in the buffer provided as an argument.
If, for some reason, the length byte of the received packet indicates
a length >127 (which is impossible), only the first two bytes will
be received.

\param [out] writeToBuffer The buffer to which to write the bytes.
*/
void radio_getReceivedFrame(OpenQueueEntry_t* writeToBuffer) {
   uint8_t temp_crc_reg_value;
   
   // initialize the buffer
   writeToBuffer->payload = &(writeToBuffer->packet[0]);
   
   // check if CRC is correct
   temp_crc_reg_value = spi_read_register(RG_PHY_RSSI);
   writeToBuffer->l1_rssi =  temp_crc_reg_value & 0x1F;      // last 5 lsb's are RSSI
   writeToBuffer->l1_crc  = (temp_crc_reg_value & 0x80)>>7;  // msb is whether packet passed CRC
   
   // copy packet from rx buffer in radio over SPI
   spi_read_buffer(writeToBuffer,2); // first read only 2 bytes to receive the length
   writeToBuffer->length = writeToBuffer->payload[1];
   if (writeToBuffer->length<=127) {
      // then retrieve whole packet (including 1B SPI address, 1B length, 1B LQI)
      spi_read_buffer(writeToBuffer,1+1+writeToBuffer->length+1);
      // shift by 2B "header" (answer received when MSP sent SPI address, 1B length). Length does not change.
      writeToBuffer->payload += 2;
      // read 1B "footer" (LQI) and store that information
      writeToBuffer->l1_lqi = writeToBuffer->payload[writeToBuffer->length];
   }
}

//======= Turning radio off

/**
\brief Turn the radio off.

This does not turn the radio entirely off, i.e. it is still listening for
commands over SPI. It does, however, make sure the power hungry components
such as the PLL are switched off.
*/
void radio_rfOff() {
   // change state
   radio_vars.state = RADIOSTATE_TURNING_OFF;
   
   // turn radio off
   spi_write_register(RG_TRX_STATE, CMD_FORCE_TRX_OFF);
   while((spi_read_register(RG_TRX_STATUS) & 0x1F) != TRX_OFF); // busy wait until done
   
   // wiggle debug pin
   DEBUG_PIN_RADIO_CLR();
   
   // change state
   radio_vars.state = RADIOSTATE_RFOFF;
}

//=========================== private =========================================

//=========================== interrupt handlers ==============================

/**
\brief Radio interrupt handler function.

this function is called by the scheduler when the radio pulses
the IRQ_RF pin. This function will check what caused the interrupt
by checking the contents of the RG_IRQ_STATUS register off the radio.
Reading this register also lowers the IRQ_RF pin.
*/
void isr_radio() {
   uint8_t capturedTime;
   uint8_t irq_status;
   // capture the time
   capturedTime = ieee154etimer_getCapturedTime();
   // reading IRQ_STATUS causes IRQ_RF (P1.6) to go low
   irq_status = spi_read_register(RG_IRQ_STATUS);
   switch (irq_status) {
      case AT_IRQ_RX_START:
         // change state
         radio_vars.state = RADIOSTATE_RECEIVING;
         // call MAC layer
         ieee154e_startOfFrame(capturedTime);
         break;
      case AT_IRQ_TRX_END:
         // change state
         radio_vars.state = RADIOSTATE_TXRX_DONE;
         // call MAC layer
         ieee154e_endOfFrame(capturedTime);
         break;
      default:
         openserial_printError(COMPONENT_PACKETFUNCTIONS,ERR_WRONG_IRQ_STATUS,
            (errorparameter_t)irq_status,
            (errorparameter_t)0);
         break;
   }
}