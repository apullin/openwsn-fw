/**
\brief Forwarding engine

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/

#include "openwsn.h"
#include "forwarding.h"
#include "iphc.h"
#include "openqueue.h"
#include "openserial.h"
#include "idmanager.h"
#include "packetfunctions.h"
#include "neighbors.h"
#include "icmpv6.h"
#include "udp.h"
#include "tcp.h"

//=========================== variables =======================================

//=========================== prototypes ======================================

void getNextHop(open_addr_t* destination, open_addr_t* addressToWrite);
error_t fowarding_send_internal(OpenQueueEntry_t *msg);

//=========================== public ==========================================

void forwarding_init() {
}

error_t forwarding_send(OpenQueueEntry_t *msg) {
   msg->owner = COMPONENT_FORWARDING;
   return fowarding_send_internal(msg);
}

void forwarding_sendDone(OpenQueueEntry_t* msg, error_t error) {
   msg->owner = COMPONENT_FORWARDING;
   if (msg->creator==COMPONENT_RADIO) {//that was a packet I had relayed
      openqueue_freePacketBuffer(msg);
   } else {//that was a packet coming from above
      switch(msg->l4_protocol) {
      case IANA_TCP:
         tcp_sendDone(msg,error);
         break;
      case IANA_UDP:
         udp_sendDone(msg,error);
         break;
      case IANA_ICMPv6:
         icmpv6_sendDone(msg,error);
         break;
      default:
         openserial_printError(COMPONENT_FORWARDING,ERR_WRONG_TRAN_PROTOCOL,msg->l4_protocol,0);
      }
   }
}

void forwarding_receive(OpenQueueEntry_t* msg, ipv6_header_iht ipv6_header) {
   msg->owner = COMPONENT_FORWARDING;
   msg->l4_protocol = ipv6_header.next_header;
   if (idmanager_isMyAddress(&ipv6_header.dest) || packetfunctions_isBroadcastMulticast(&ipv6_header.dest)) {//for me
      memcpy(&(msg->l3_destinationORsource),&ipv6_header.src,sizeof(open_addr_t));
      switch(msg->l4_protocol) {
      case IANA_TCP:
         tcp_receive(msg);
         break;
      case IANA_UDP:
         udp_receive(msg);
         break;
      case IANA_ICMPv6:
         icmpv6_receive(msg);
         break;
      default:
         openserial_printError(COMPONENT_FORWARDING,ERR_WRONG_TRAN_PROTOCOL,msg->l4_protocol,0);
      }
   } else { //relay
      memcpy(&(msg->l3_destinationORsource),&ipv6_header.dest,sizeof(open_addr_t));//because initially contains source
      //TBC: source address gets changed!
      //resend as if from upper layer
      if (fowarding_send_internal(msg)==E_FAIL) {
         openqueue_freePacketBuffer(msg);
      }
   }
}

bool forwarding_debugPrint() {
   return FALSE;
}

//=========================== private =========================================

error_t fowarding_send_internal(OpenQueueEntry_t *msg) {
   getNextHop(&(msg->l3_destinationORsource),&(msg->l2_nextORpreviousHop));
   if (msg->l2_nextORpreviousHop.type==ADDR_NONE) {
      openserial_printError(COMPONENT_FORWARDING,ERR_NO_NEXTHOP,0,0);
      return E_FAIL;
   }
   return iphc_sendFromForwarding(msg);
}

void getNextHop(open_addr_t* destination128b, open_addr_t* addressToWrite64b) {
/*   uint8_t i;
   open_addr_t temp_prefix64btoWrite;
   if (packetfunctions_isBroadcastMulticast(destination128b)) {
      addressToWrite64b->type = ADDR_64B;
      for (i=0;i<8;i++) {
         addressToWrite64b->addr_64b[i] = 0xff;
      }
   } else if (neighbors_isStableNeighbor(destination128b)) {    //destination is 1-hop neighbor
      packetfunctions_ip128bToMac64b(destination128b,&temp_prefix64btoWrite,addressToWrite64b);
   } else {
      neighbors_getPreferredParent(addressToWrite64b,ADDR_64B); //destination is remote
   }*/
   //poipoi
   addressToWrite64b->type = ADDR_64B;
   addressToWrite64b->addr_64b[0] = 0x14;
   addressToWrite64b->addr_64b[1] = 0x15;
   addressToWrite64b->addr_64b[2] = 0x92;
   addressToWrite64b->addr_64b[3] = 0x09;
   addressToWrite64b->addr_64b[4] = 0x02;
   addressToWrite64b->addr_64b[5] = 0x2c;
   addressToWrite64b->addr_64b[6] = 0x00;
   addressToWrite64b->addr_64b[7] = 0x87;
}