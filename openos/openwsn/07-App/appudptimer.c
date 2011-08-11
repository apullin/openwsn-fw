/*
 * UDP Timer application
 * 
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 * 
 */

#include "openwsn.h"
#include "appudptimer.h"
#include "udp.h"
#include "openqueue.h"
#include "openserial.h"
#include "packetfunctions.h"
#include "timers.h"

//===================================== variables =============================

//===================================== prototypes ============================

//===================================== public ================================

void appudptimer_init() {
    timer_startPeriodic(TIMER_UDP_TIMER,32768);
}

void appudptimer_trigger() {
   OpenQueueEntry_t* pkt;

   //prepare packet
   pkt = openqueue_getFreePacketBuffer();
   if (pkt==NULL) {
      openserial_printError(COMPONENT_APPUDPTIMER,ERR_NO_FREE_PACKET_BUFFER,(errorparameter_t)0,(errorparameter_t)0);
      return;
   }
   pkt->creator                     = COMPONENT_APPUDPTIMER;
   pkt->owner                       = COMPONENT_APPUDPTIMER;
   pkt->l4_protocol                 = IANA_UDP;
   pkt->l4_sourcePortORicmpv6Type   = WKP_UDP_TIMER;
   pkt->l4_destination_port         = 0xabcd;
   pkt->l3_destinationORsource.type = ADDR_128B;
   pkt->l3_destinationORsource.addr_128b[ 0] = 0xde;
   pkt->l3_destinationORsource.addr_128b[ 1] = 0xad;
   pkt->l3_destinationORsource.addr_128b[ 2] = 0xbe;
   pkt->l3_destinationORsource.addr_128b[ 3] = 0xef;
   pkt->l3_destinationORsource.addr_128b[ 4] = 0xfa;
   pkt->l3_destinationORsource.addr_128b[ 5] = 0xce;
   pkt->l3_destinationORsource.addr_128b[ 6] = 0xca;
   pkt->l3_destinationORsource.addr_128b[ 7] = 0xfe;
   pkt->l3_destinationORsource.addr_128b[ 8] = 0xde;
   pkt->l3_destinationORsource.addr_128b[ 9] = 0xad;
   pkt->l3_destinationORsource.addr_128b[10] = 0xbe;
   pkt->l3_destinationORsource.addr_128b[11] = 0xef;
   pkt->l3_destinationORsource.addr_128b[12] = 0xfa;
   pkt->l3_destinationORsource.addr_128b[13] = 0xce;
   pkt->l3_destinationORsource.addr_128b[14] = 0xca;
   pkt->l3_destinationORsource.addr_128b[15] = 0xfe;
   packetfunctions_reserveHeaderSize(pkt,6);
   ((uint8_t*)pkt->payload)[0]      = 'p';
   ((uint8_t*)pkt->payload)[1]      = 'o';
   ((uint8_t*)pkt->payload)[2]      = 'i';
   ((uint8_t*)pkt->payload)[3]      = 'p';
   ((uint8_t*)pkt->payload)[4]      = 'o';
   ((uint8_t*)pkt->payload)[5]      = 'i';
   //send packet
   if ((udp_send(pkt))==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
   }
}

void appudptimer_sendDone(OpenQueueEntry_t* msg, error_t error) {
   msg->owner = COMPONENT_APPUDPTIMER;
   if (msg->creator!=COMPONENT_APPUDPTIMER) {
      openserial_printError(COMPONENT_APPUDPTIMER,ERR_SENDDONE_FOR_MSG_I_DID_NOT_SEND,0,0);
   }
   openqueue_freePacketBuffer(msg);
}

void appudptimer_receive(OpenQueueEntry_t* msg) {
   openqueue_freePacketBuffer(msg);
}

bool appudptimer_debugPrint() {
   return FALSE;
}