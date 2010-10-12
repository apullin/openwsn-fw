/*
 * UDP Echo application
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#include "openwsn.h"
#include "appudpecho.h"
#include "udp.h"
#include "openqueue.h"
#include "openserial.h"

//===================================== variables =============================

//===================================== prototypes ============================

//===================================== public ================================

void appudpecho_init() {
}

void appudpecho_receive(OpenQueueEntry_t* msg) {
   uint16_t temp_l4_destination_port;
   msg->owner   = COMPONENT_APPUDPECHO;
   //reply with the same OpenQueueEntry_t
   msg->creator                       = COMPONENT_APPUDPECHO;
   msg->l4_protocol                   = IANA_UDP;
   temp_l4_destination_port           = msg->l4_destination_port;
   msg->l4_destination_port           = msg->l4_sourcePortORicmpv6Type;
   msg->l4_sourcePortORicmpv6Type     = temp_l4_destination_port;
   if ((udp_send(msg))==E_FAIL) {
      openqueue_freePacketBuffer(msg);
   }
}

void appudpecho_sendDone(OpenQueueEntry_t* msg, error_t error) {
   msg->owner = COMPONENT_APPUDPECHO;
   if (msg->creator!=COMPONENT_APPUDPECHO) {
      openserial_printError(COMPONENT_APPUDPECHO,ERR_SENDDONE_FOR_MSG_I_DID_NOT_SEND,0,0);
   }
   openqueue_freePacketBuffer(msg);
}

bool appudpecho_debugPrint() {
   return FALSE;
}
