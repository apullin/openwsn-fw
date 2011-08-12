/*
 * Ohlone, world's smaller web server
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 * Ankur Mehta <mehtank@eecs.berkeley.edu>, September 2010
 */

#include "openwsn.h"
#include "apptcpohlone.h"
#include "openqueue.h"
#include "packetfunctions.h"
#include "openserial.h"
#include "tcp.h"

#include "ohlone_webpages.h"

//===================================== variables =============================

OpenQueueEntry_t* apptcpohlone_pkt;
bool              apptcpohlone_sending;
uint16_t          apptcpohlone_httpChunk;
uint8_t           apptcpohlone_getRequest[TCP_DEFAULT_WINDOW_SIZE];

//===================================== prototypes ============================

void apptcpohlone_sendpkt();
bool apptcpohlone_check4chars(uint8_t c1[4], uint8_t c2[4]);

//===================================== public ================================

void apptcpohlone_init() {
   apptcpohlone_httpChunk = 0;
   apptcpohlone_getRequest[0] = '/';
   apptcpohlone_getRequest[1] = ' ';
   ohlone_webpages_init();
}

bool apptcpohlone_shouldIlisten() {
   return TRUE;
}

void apptcpohlone_sendpkt() {
   uint8_t buffer[TCP_DEFAULT_WINDOW_SIZE];
   uint8_t buffer_len;
  
   buffer_len = ohlone_webpage(apptcpohlone_getRequest, apptcpohlone_httpChunk++, buffer);
   
   if (buffer_len == 0) {
      // No more to send
      // close TCP session, but keep listening
      apptcpohlone_getRequest[0] = '/';
      apptcpohlone_getRequest[1] = ' ';
      tcp_close();
      return;
   }

   apptcpohlone_pkt = openqueue_getFreePacketBuffer();
   if (apptcpohlone_pkt==NULL) {
      openserial_printError(COMPONENT_APPTCPOHLONE,ERR_NO_FREE_PACKET_BUFFER,(errorparameter_t)0,(errorparameter_t)0);
      tcp_close();
      return;
   }
   apptcpohlone_pkt->creator = COMPONENT_APPTCPOHLONE;
   apptcpohlone_pkt->owner   = COMPONENT_APPTCPOHLONE;
   
   packetfunctions_reserveHeaderSize(apptcpohlone_pkt, buffer_len);
   memcpy(apptcpohlone_pkt->payload, buffer, buffer_len);
   
   if ((tcp_send(apptcpohlone_pkt))==E_FAIL) {
      openqueue_freePacketBuffer(apptcpohlone_pkt);
      tcp_close();
   }

}

bool apptcpohlone_check4chars(uint8_t c1[4], uint8_t c2[4]) {
  return ((c1[0] == c2[0]) && 
          (c1[1] == c2[1]) && 
          (c1[2] == c2[2]) && 
          (c1[3] == c2[3]));
}

void apptcpohlone_receive(OpenQueueEntry_t* msg) {
   uint8_t payload_index;
   
   for (payload_index=0;payload_index<msg->length-3;payload_index++) {
      if (apptcpohlone_check4chars(msg->payload+payload_index, "GET "))
         memcpy(apptcpohlone_getRequest, 
                msg->payload + payload_index + 4, 
                msg->length - payload_index - 4);

      if (apptcpohlone_check4chars(msg->payload+payload_index, "\r\n\r\n")) {
         apptcpohlone_httpChunk = 0;
         apptcpohlone_sendpkt();
         return;
      }
   }
   openserial_printData((uint8_t*)(msg->payload),msg->length);
   openqueue_freePacketBuffer(msg);
}

void apptcpohlone_sendDone(OpenQueueEntry_t* msg, error_t error) {
   msg->owner = COMPONENT_APPTCPOHLONE;
   if (msg->creator!=COMPONENT_APPTCPOHLONE) {
      openserial_printError(COMPONENT_APPTCPOHLONE,ERR_UNEXPECTED_SENDDONE,0,0);
   }
   
   apptcpohlone_sendpkt();
   openqueue_freePacketBuffer(msg);
}

void apptcpohlone_connectDone(error_t error) {
}

bool apptcpohlone_debugPrint() {
   return FALSE;
}