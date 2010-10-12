/*
 * UDP LEDs application
 *
 * Authors:
 * Ankur Mehta <mehtank@eecs.berkeley.edu>, September 2010
 */

#include "openwsn.h"
#include "appudpleds.h"
//openwsn stack
#include "udp.h"
#include "openqueue.h"
#include "openserial.h"
#include "packetfunctions.h"

//===================================== variables =============================

//===================================== prototypes ============================

//===================================== public ================================

void appudpleds_init() {
   P2DIR  |=  0x0F;                              // P2.0-3 output
   P2OUT  &= ~0x0F;                              // P2OUT = 0bxxxx0000
}

//this is called when the corresponding button is pressed on the OpenVisualizer interface
void appudpleds_trigger() {
  int i = 0;
  P2OUT ^= 0x0f;
  while (--i);
  P2OUT ^= 0x0f;
  while (--i);
  P2OUT ^= 0x0f;
  while (--i);
  P2OUT ^= 0x0f;
}

//I just received a request
void appudpleds_receive(OpenQueueEntry_t* msg) {
   msg->owner = COMPONENT_APPUDPLEDS;
   if (msg->length==1) {
      P2OUT &= ~0x0f;                            // turn off all 4 LEDs
      P2OUT |= (0x0f &  msg->payload[0]);        // turn on the ones we want
   }
   openqueue_freePacketBuffer(msg);
}

void appudpleds_sendDone(OpenQueueEntry_t* msg, error_t error) {
   msg->owner = COMPONENT_APPUDPLEDS;
   if (msg->creator!=COMPONENT_APPUDPLEDS) {
      openserial_printError(COMPONENT_APPUDPLEDS,ERR_SENDDONE_FOR_MSG_I_DID_NOT_SEND,0,0);
   }
   openqueue_freePacketBuffer(msg);
}

bool appudpleds_debugPrint() {
   return FALSE;
}