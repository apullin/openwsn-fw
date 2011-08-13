/*
 * ICMPv6 RPL implementation
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#include "openwsn.h"
#include "icmpv6rpl.h"
#include "icmpv6.h"
#include "openserial.h"
#include "openqueue.h"
#include "timers.h"
#include "neighbors.h"
#include "packetfunctions.h"

//===================================== variables =============================

uint16_t res_periodDIO;
uint8_t  res_delayDIO;
__no_init volatile uint8_t random_uint8 @ 0x10c0;
open_addr_t all_routers_multicast;

bool        icmpv6rpl_busySending;
uint16_t    icmpv6rpl_seq=0;

//===================================== prototypes ============================

void sendDIO();

//===================================== public ================================

void icmpv6rpl_init() {
   icmpv6rpl_busySending = FALSE;
   res_periodDIO = 40000+(64*(*(&random_uint8)));       // pseudo-random
   all_routers_multicast.type = ADDR_128B;
   all_routers_multicast.addr_128b[0]  = 0xff;
   all_routers_multicast.addr_128b[1]  = 0x02;
   all_routers_multicast.addr_128b[2]  = 0x00;
   all_routers_multicast.addr_128b[3]  = 0x00;
   all_routers_multicast.addr_128b[4]  = 0x00;
   all_routers_multicast.addr_128b[5]  = 0x00;
   all_routers_multicast.addr_128b[6]  = 0x00;
   all_routers_multicast.addr_128b[7]  = 0x00;
   all_routers_multicast.addr_128b[8]  = 0x00;
   all_routers_multicast.addr_128b[9]  = 0x00;
   all_routers_multicast.addr_128b[10] = 0x00;
   all_routers_multicast.addr_128b[11] = 0x00;
   all_routers_multicast.addr_128b[12] = 0x00;
   all_routers_multicast.addr_128b[13] = 0x00;
   all_routers_multicast.addr_128b[14] = 0x00;
   all_routers_multicast.addr_128b[15] = 0x02;
   //poipoi disabling timer_startPeriodic(TIMER_RPL,res_periodDIO);
}

void icmpv6rpl_trigger() {
   uint8_t number_bytes_from_input_buffer;
   uint8_t input_buffer[16];
   //get command from OpenSerial (16B IPv6 destination address)
   number_bytes_from_input_buffer = openserial_getInputBuffer(&(input_buffer[0]),sizeof(input_buffer));
   if (number_bytes_from_input_buffer!=sizeof(input_buffer)) {
      openserial_printError(COMPONENT_ICMPv6ECHO,ERR_INPUTBUFFER_LENGTH,
            (errorparameter_t)number_bytes_from_input_buffer,
            (errorparameter_t)0);
      return;
   };
   //send
   sendDIO();
}

void icmpv6rpl_sendDone(OpenQueueEntry_t* msg, error_t error) {
   msg->owner = COMPONENT_ICMPv6RPL;
   if (msg->creator!=COMPONENT_ICMPv6RPL) {//that was a packet I had not created
      openserial_printError(COMPONENT_ICMPv6RPL,ERR_UNEXPECTED_SENDDONE,0,0);
   }
   openqueue_freePacketBuffer(msg);
   icmpv6rpl_busySending = FALSE;
}

void icmpv6rpl_receive(OpenQueueEntry_t* msg) {
   msg->owner = COMPONENT_ICMPv6RPL;
   //toss ICMPv6 header
   packetfunctions_tossHeader(msg,sizeof(ICMPv6_ht));
   //update neighbor table
   neighbors_receiveDIO(msg);
   //free packet
   openqueue_freePacketBuffer(msg);
}

bool icmpv6rpl_debugPrint() {
   return FALSE;
}

//===================================== public (timer) ========================

void timer_rpl_fired() {
   res_delayDIO = (res_delayDIO+1)%5; //send on average every 10s
   if (res_delayDIO==0) {
      sendDIO();
      //set a new random periodDIO
      res_periodDIO = 30000+(128*(*(&random_uint8)));       // pseudo-random
      timer_startPeriodic(TIMER_RPL,res_periodDIO);
   }
}

//===================================== private ===============================

void sendDIO() {
   OpenQueueEntry_t* msg;
   if (icmpv6rpl_busySending==TRUE) {
      openserial_printError(COMPONENT_ICMPv6RPL,ERR_BUSY_SENDING,0,0);
   } else {
      icmpv6rpl_busySending = TRUE;
      msg = openqueue_getFreePacketBuffer();
      if (msg==NULL) {
         openserial_printError(COMPONENT_ICMPv6RPL,ERR_NO_FREE_PACKET_BUFFER,(errorparameter_t)0,(errorparameter_t)0);
         return;
      }
      //admin
      msg->creator                               = COMPONENT_ICMPv6RPL;
      msg->owner                                 = COMPONENT_ICMPv6RPL;
      //l4
      msg->l4_protocol                           = IANA_ICMPv6;
      msg->l4_sourcePortORicmpv6Type             = IANA_ICMPv6_RPL;
      //l3
      memcpy(&(msg->l3_destinationORsource),&all_routers_multicast,sizeof(open_addr_t));
      //payload
      packetfunctions_reserveHeaderSize(msg,1);
      *((uint8_t*)(msg->payload)) = neighbors_getMyDAGrank();
      //ICMPv6 header
      packetfunctions_reserveHeaderSize(msg,sizeof(ICMPv6_ht));
      ((ICMPv6_ht*)(msg->payload))->type         = msg->l4_sourcePortORicmpv6Type;
      ((ICMPv6_ht*)(msg->payload))->code         = IANA_ICMPv6_RPL_DIO;
      packetfunctions_htons(0x1234,(uint8_t*)&((ICMPv6_ht*)(msg->payload))->identifier);
      packetfunctions_htons(icmpv6rpl_seq++ ,(uint8_t*)&((ICMPv6_ht*)(msg->payload))->sequence_number); 
      packetfunctions_calculateChecksum(msg,(uint8_t*)&(((ICMPv6_ht*)(msg->payload))->checksum));//call last
      //send
      if (icmpv6_send(msg)!=E_SUCCESS) {
         icmpv6rpl_busySending = FALSE;
         openqueue_freePacketBuffer(msg);
      }
   }
}