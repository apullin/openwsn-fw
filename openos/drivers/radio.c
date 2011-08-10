#include "gina_config.h"
#include "radio.h"
#include "packetfunctions.h"
#include "openqueue.h"
#include "IEEE802154E.h"

//===================================== variables ==============================

uint8_t radio_state;
bool    isStartOfFrameEvent;

//=========================== prototypes =======================================

//=========================== initialize the radio =============================

/**
\brief Initialize the radio.
*/
void radio_init() {
   // set the radio debug pin as output
   DEBUG_PIN_RADIO_INIT();
   
   // the first interrupt I'll receive will be a start of frame
   isStartOfFrameEvent = TRUE;
   
   // initialize radio state
   radio_state = RADIO_STATE_STOPPED;
   
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
   
   // configure radio
   spi_write_register(RG_TRX_STATE, CMD_FORCE_TRX_OFF);  // turn radio off
   spi_write_register(RG_IRQ_MASK, 0x0C);                // fire interrupt on TRX_END and RX_START
   spi_read_register(RG_IRQ_STATUS);                     // deassert the interrupt pin (P1.6) in case is high
   spi_write_register(RG_ANT_DIV, USE_CHIP_ANTENNA);     // use chip antenna
#define RG_TRX_CTRL_1 0x04
   spi_write_register(RG_TRX_CTRL_1, 0x20);              // have the radio calculate CRC
   
   while((spi_read_register(RG_TRX_STATUS) & 0x1F) != TRX_OFF); //busy wait until radio status is TRX_OFF
   radio_state = RADIO_STATE_STARTED;
}

//=========================== sending a packet ================================

void radio_setFrequency(uint8_t frequency) {
   radio_state = RADIO_STATE_SETTING_FREQUENCY;
   if (frequency < 11 || frequency > 26){
      frequency = 26;
   }
   spi_write_register(RG_PHY_CC_CCA,0x20+frequency);
}

void radio_loadPacket(OpenQueueEntry_t* packet) {
   // don't declare radio as owner or else MAC will not be able to retransmit
   
   // add 1B length at the beginning (PHY header)
   packetfunctions_reserveHeaderSize(packet,1);
   packet->payload[0] = packet->length-1;   // length (not counting length field)
   
   // add 1B SPI address at the beginning (internally for SPI)
   packetfunctions_reserveHeaderSize(packet,1);
   packet->payload[0] = 0x60;
   
   // load packet in TXFIFO
   radio_state = RADIO_STATE_LOADING_PACKET;
   spi_write_buffer(packet);
   radio_state = RADIO_STATE_READY_TX;
}

void radio_txEnable() {
   // turn on radio's PLL
   DEBUG_PIN_RADIO_SET();
   radio_state = RADIO_STATE_TRANSMITTING;
   spi_write_register(RG_TRX_STATE, CMD_PLL_ON);
   
   // busy wait until radio status is PLL_ON
   while((spi_read_register(RG_TRX_STATUS) & 0x1F) != PLL_ON);
}

void radio_txNow(){
   //send packet by pulsing the RF_SLP_TR_CNTL pin
   P4OUT |=  0x80;
   P4OUT &= ~0x80;
}

//=========================== receiving a packet ==============================

void radio_rxEnable() {
   // put radio in reception mode
   spi_write_register(RG_TRX_STATE, CMD_RX_ON);
   DEBUG_PIN_RADIO_SET();
   
   //busy wait until radio status is PLL_ON
   while((spi_read_register(RG_TRX_STATUS) & 0x1F) != RX_ON);
   radio_state = RADIO_STATE_READY_RX;
   
   // clear timestamp overflow bit (used for timestamping incoming packets)
   CLEAR_TIMER_OVERFLOW();
}

void radio_rxNow() {
   // this function doesn't do anything since this radio is already
   // listening after radio_rxEnable() is called.
}

void radio_getReceivedFrame(OpenQueueEntry_t* writeToBuffer) {
   uint8_t temp_crc_reg_value;
   // initialize the buffer
   writeToBuffer->payload = &(writeToBuffer->packet[0]);
   // check if CRC is correct
   temp_crc_reg_value = spi_read_register(RG_PHY_RSSI);
   writeToBuffer->l1_rssi =  temp_crc_reg_value & 0x1F;      // last 5 lsb's are RSSI
   writeToBuffer->l1_crc  = (temp_crc_reg_value & 0x80)>>7;  // msb is whether packet passed CRC
   // copy packet from rx buffer in radio over SPI
   // first read only 2 bytes to receive the length
   spi_read_buffer(writeToBuffer,2);
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

//=========================== turning radio off ===============================

void radio_rfOff() {
   // turn radio off
   spi_write_register(RG_TRX_STATE, CMD_FORCE_TRX_OFF);
   
   // busy wait until radio status is TRX_OFF
   while((spi_read_register(RG_TRX_STATUS) & 0x1F) != TRX_OFF);
   
   // lower the radio debug pin
   DEBUG_PIN_RADIO_CLR();
   
   // write local variable
   radio_state = RADIO_STATE_STARTED;
}

//=========================== interrupt handler ================================

void isr_radio() {
   uint8_t irq_status;
   // reading IRQ_STATUS causes IRQ_RF (P1.6) to go low
   irq_status = spi_read_register(RG_IRQ_STATUS);
   switch (irq_status) {
      case AT_IRQ_RX_START:
         ieee154e_startOfFrame();
         break;
      case AT_IRQ_TRX_END:
         ieee154e_endOfFrame();
         break;
      default:
         // print error poipoipoi
         break;
   }
}