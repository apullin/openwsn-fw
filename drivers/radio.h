#ifndef __RADIO_H
#define __RADIO_H

#include "msp430x26x.h"
#include "atmel.h"
#include "spi.h"

//===================================== define ================================

//radio states (at_state)
#define RADIO_STATE_STOPPED            0x00
#define RADIO_STATE_STARTED            0x03
#define RADIO_STATE_LOADING_PACKET     0x04
#define RADIO_STATE_SETTING_CHANNEL    0x05
#define RADIO_STATE_READY_TX           0x06
#define RADIO_STATE_TRANSMITTING       0x07
#define RADIO_STATE_READY_RX           0x08
#define RADIO_STATE_RECEIVING          0x09
#define RADIO_STATE_STOPPING           0x10

#define USE_WIRE_ANTENNA               0x06
#define USE_CHIP_ANTENNA               0x05

//===================================== prototypes ============================

void    radio_init();                      // configures both MSP and radio chip
error_t radio_send(OpenQueueEntry_t* packet);    // send a packet
void    radio_sleep();
void    radio_rxOn(uint8_t channel);             // set the radio in reception mode
void    isr_radio();
void    radio_packet_received(OpenQueueEntry_t* packetReceived); // called when a packet is completely received
void    radio_rfOff();                           // switch radio off

//bk april 2011
error_t radio_prepare_send(OpenQueueEntry_t* packet);
error_t radio_send_now();

#endif
