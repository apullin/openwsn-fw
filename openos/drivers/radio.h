#ifndef __RADIO_H
#define __RADIO_H

#include "msp430x26x.h"
#include "atmel.h"
#include "spi.h"

//===================================== define ================================

// after you get an interrupt from the radio, read
// the status register to know what type it is, amoung
// the following
#define AT_IRQ_BAT_LOW       0x80 // supply voltage below the programmed threshold
#define AT_IRQ_TRX_UR        0x40 // frame Buffer access violation
#define AT_IRQ_AMI           0x20 // address matching
#define AT_IRQ_CCA_ED_DONE   0x10 // end of a CCA or ED measurement
#define AT_IRQ_TRX_END       0x08 // completion of a frame transmission/reception
#define AT_IRQ_RX_START      0x04 // start of a PSDU reception
#define AT_IRQ_PLL_UNLOCK    0x02 // PLL unlock
#define AT_IRQ_PLL_LOCK      0x01 // PLL lock

//radio states (at_state)
#define RADIO_STATE_STOPPED            0x00
#define RADIO_STATE_STARTED            0x03
#define RADIO_STATE_LOADING_PACKET     0x04
#define RADIO_STATE_SETTING_FREQUENCY  0x05
#define RADIO_STATE_READY_TX           0x06
#define RADIO_STATE_TRANSMITTING       0x07
#define RADIO_STATE_READY_RX           0x08
#define RADIO_STATE_RECEIVING          0x09
#define RADIO_STATE_STOPPING           0x10

#define USE_WIRE_ANTENNA               0x06
#define USE_CHIP_ANTENNA               0x05

#define CLEAR_TIMER_OVERFLOW() TBCCTL5 &= ~(CCIFG|COV)

//===================================== prototypes ============================

void radio_init();
void radio_setFrequency(uint8_t frequency);
void radio_loadPacket(OpenQueueEntry_t* packet);
void radio_txEnable();
void radio_txNow();
void radio_rxEnable();
void radio_rxNow();
void radio_getReceivedFrame(OpenQueueEntry_t* writeToBuffer);
void radio_rfOff();

void isr_radio();
#endif
