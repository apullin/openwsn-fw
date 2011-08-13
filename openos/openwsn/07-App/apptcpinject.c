/**
\brief TCP Inject application

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/

#include "openwsn.h"
#include "apptcpinject.h"
#include "openserial.h"
#include "packetfunctions.h"
#include "tcp.h"
#include "openqueue.h"

//=========================== variables =======================================

OpenQueueEntry_t* apptcpinject_pkt;
bool              apptcpinject_sending;
open_addr_t       apptcpinject_hisAddress;
uint16_t          apptcpinject_hisPort;

//=========================== prototypes ======================================

//=========================== public ==========================================

void apptcpinject_init() {
}

bool apptcpinject_shouldIlisten() {
   return FALSE;
}

void apptcpinject_trigger() {
   uint8_t number_bytes_from_input_buffer;
   uint8_t input_buffer[18];
   //get command from OpenSerial (16B IPv6 destination address, 2B destination port)
   number_bytes_from_input_buffer = openserial_getInputBuffer(&(input_buffer[0]),sizeof(input_buffer));
   if (number_bytes_from_input_buffer!=sizeof(input_buffer)) {
      openserial_printError(COMPONENT_APPTCPINJECT,ERR_INPUTBUFFER_LENGTH,
            (errorparameter_t)number_bytes_from_input_buffer,
            (errorparameter_t)0);
      return;
   };
   apptcpinject_hisAddress.type = ADDR_128B;
   memcpy(&(apptcpinject_hisAddress.addr_128b[0]),&(input_buffer[0]),16);
   apptcpinject_hisPort = packetfunctions_ntohs(&(input_buffer[16]));
   //connect
   tcp_connect(&apptcpinject_hisAddress,apptcpinject_hisPort,WKP_TCP_INJECT);
}

void apptcpinject_connectDone(error_t error) {
   if (error==E_SUCCESS) {
      apptcpinject_pkt = openqueue_getFreePacketBuffer();
      if (apptcpinject_pkt==NULL) {
         openserial_printError(COMPONENT_APPTCPINJECT,ERR_NO_FREE_PACKET_BUFFER,(errorparameter_t)0,(errorparameter_t)0);
         return;
      }
      apptcpinject_pkt->creator                      = COMPONENT_APPTCPINJECT;
      apptcpinject_pkt->owner                        = COMPONENT_APPTCPINJECT;
      apptcpinject_pkt->l4_protocol                  = IANA_UDP;
      apptcpinject_pkt->l4_sourcePortORicmpv6Type    = WKP_TCP_INJECT;
      apptcpinject_pkt->l4_destination_port          = apptcpinject_hisPort;
      memcpy(&(apptcpinject_pkt->l3_destinationORsource),&apptcpinject_hisAddress,sizeof(open_addr_t));
      packetfunctions_reserveHeaderSize(apptcpinject_pkt,6);
      ((uint8_t*)apptcpinject_pkt->payload)[0] = 'p';
      ((uint8_t*)apptcpinject_pkt->payload)[1] = 'o';
      ((uint8_t*)apptcpinject_pkt->payload)[2] = 'i';
      ((uint8_t*)apptcpinject_pkt->payload)[3] = 'p';
      ((uint8_t*)apptcpinject_pkt->payload)[4] = 'o';
      ((uint8_t*)apptcpinject_pkt->payload)[5] = 'i';
      if (tcp_send(apptcpinject_pkt)==E_FAIL) {
         openqueue_freePacketBuffer(apptcpinject_pkt);
      }
      return;
   }
}

void apptcpinject_sendDone(OpenQueueEntry_t* msg, error_t error) {
   msg->owner = COMPONENT_APPTCPINJECT;
   if (msg->creator!=COMPONENT_APPTCPINJECT) {
      openserial_printError(COMPONENT_APPTCPINJECT,ERR_UNEXPECTED_SENDDONE,0,0);
   }
   tcp_close();
   openqueue_freePacketBuffer(msg);
}

void apptcpinject_receive(OpenQueueEntry_t* msg) {
}

bool apptcpinject_debugPrint() {
   return FALSE;
}

//=========================== private =========================================