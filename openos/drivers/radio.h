/**
\brief Radio driver for the Atmel AT86RF231 IEEE802.15.4-compliant radio

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
*/

#ifndef __RADIO_H
#define __RADIO_H

/**
\addtogroup drivers
\{
\addtogroup Radio
\{
*/

#include "openwsn.h"
#include "msp430x26x.h"
#include "atmel.h"

//=========================== define ==========================================

/**
\brief Possible values for the status of the radio.

After you get an interrupt from the radio, read the status register
(RG_IRQ_STATUS) to know what type it is, amoung the following.
*/
enum radio_irqstatus_enum {
   AT_IRQ_BAT_LOW                 = 0x80,   // supply voltage below the programmed threshold
   AT_IRQ_TRX_UR                  = 0x40,   // frame buffer access violation
   AT_IRQ_AMI                     = 0x20,   // address matching
   AT_IRQ_CCA_ED_DONE             = 0x10,   // end of a CCA or ED measurement
   AT_IRQ_TRX_END                 = 0x08,   // completion of a frame transmission/reception
   AT_IRQ_RX_START                = 0x04,   // start of a PSDU reception
   AT_IRQ_PLL_UNLOCK              = 0x02,   // PLL unlock
   AT_IRQ_PLL_LOCK                = 0x01,   // PLL lock
};

/**
\brief States of the radio FSM.

The radio driver is very minimal in that it does not follow a state machine.
It is up to the MAC layer to ensure that the different radio operations 
are called in the righr order.

The radio keeps a state for debugging purposes only.
*/
enum radio_state_enum {
   RADIOSTATE_STOPPED             = 0x00,
   RADIOSTATE_RFOFF               = 0x01,
   RADIOSTATE_SETTING_FREQUENCY   = 0x02,
   RADIOSTATE_FREQUENCY_SET       = 0x03,
   RADIOSTATE_LOADING_PACKET      = 0x04,
   RADIOSTATE_PACKET_LOADED       = 0x05,
   RADIOSTATE_ENABLING_TX         = 0x06,
   RADIOSTATE_TX_ENABLED          = 0x07,
   RADIOSTATE_TRANSMITTING        = 0x08,
   RADIOSTATE_ENABLING_RX         = 0x09,
   RADIOSTATE_LISTENING           = 0x0a,
   RADIOSTATE_RECEIVING           = 0x0b,
   RADIOSTATE_TXRX_DONE           = 0x0c,
   RADIOSTATE_TURNING_OFF         = 0x0d,
};

/**
\brief Setting for which antenna to use

The following setting are the options which can be written
in the radio's RG_ANT_DIV register, which sets which of the 
two antennas to use.
*/
enum radio_antennaselection_enum {
   RADIO_UFL_ANTENNA              = 0x06,   // always use the U.FL antenna
   RADIO_CHIP_ANTENNA             = 0x05,   // always use the chip antenna
};

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

// called from the MAC layer
void radio_init();
void radio_setFrequency(uint8_t frequency);
void radio_loadPacket(OpenQueueEntry_t* packet);
void radio_txEnable();
void radio_txNow();
void radio_rxEnable();
void radio_rxNow();
void radio_getReceivedFrame(OpenQueueEntry_t* writeToBuffer);
void radio_rfOff();

// interrupt handlers
void isr_radio();

/**
\}
\}
*/

#endif
