/**
\brief TCP implementation (as per http://tools.ietf.org/html/rfc793)

See http://openwsn.berkeley.edu/wiki/OpenTcp for state machine and documentation.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/

#include "openwsn.h"
#include "tcp.h"
#include "openserial.h"
#include "openqueue.h"
#include "forwarding.h"
#include "packetfunctions.h"
#include "timers.h"
//TCP applications
#include "apptcpecho.h"
#include "apptcpinject.h"
#include "apptcpohlone.h"
#include "apptcpprint.h"

//=========================== variables =======================================

uint8_t           tcp_state;
uint32_t          tcp_mySeqNum;
uint16_t          tcp_myPort;
uint32_t          tcp_hisNextSeqNum;
uint16_t          tcp_hisPort;
open_addr_t       tcp_hisIPv6Address;
OpenQueueEntry_t* tcp_dataToSend;
OpenQueueEntry_t* tcp_dataReceived;

//=========================== prototypes ======================================

void prependTCPHeader(OpenQueueEntry_t* msg, bool ack, bool push, bool rst, bool syn, bool fin);
bool containsControlBits(OpenQueueEntry_t* msg, uint8_t ack, uint8_t rst, uint8_t syn, uint8_t fin);
void tcp_change_state(uint8_t new_state);
void reset();

//=========================== public ==========================================

void tcp_init() {
   reset();
}

error_t tcp_connect(open_addr_t* dest, uint16_t param_tcp_hisPort, uint16_t param_tcp_myPort) {
   //[command] establishment
   OpenQueueEntry_t* tempPkt;
   if (tcp_state!=TCP_STATE_CLOSED) {
      openserial_printError(COMPONENT_TCP,ERR_WRONG_TCP_STATE,(errorparameter_t)tcp_state,(errorparameter_t)0);
      return E_FAIL;
   }
   tcp_myPort  = param_tcp_myPort;
   tcp_hisPort = param_tcp_hisPort;
   memcpy(&tcp_hisIPv6Address,dest,sizeof(open_addr_t));
   //I receive command 'connect', I send SYNC
   tempPkt = openqueue_getFreePacketBuffer();
   if (tempPkt==NULL) {
      openserial_printError(COMPONENT_TCP,ERR_NO_FREE_PACKET_BUFFER,0,0);
      return E_FAIL;
   }
   tempPkt->creator                = COMPONENT_TCP;
   tempPkt->owner                  = COMPONENT_TCP;
   memcpy(&(tempPkt->l3_destinationORsource),&tcp_hisIPv6Address,sizeof(open_addr_t));
   tcp_mySeqNum = TCP_INITIAL_SEQNUM;
   prependTCPHeader(tempPkt,
         TCP_ACK_NO,
         TCP_PSH_NO,
         TCP_RST_NO,
         TCP_SYN_YES,
         TCP_FIN_NO);
   tcp_change_state(TCP_STATE_ALMOST_SYN_SENT);
   return forwarding_send(tempPkt);
}

error_t tcp_send(OpenQueueEntry_t* msg) {             //[command] data
   msg->owner = COMPONENT_TCP;
   if (tcp_state!=TCP_STATE_ESTABLISHED) {
      openserial_printError(COMPONENT_TCP,ERR_WRONG_TCP_STATE,(errorparameter_t)tcp_state,(errorparameter_t)2);
      return E_FAIL;
   }
   if (tcp_dataToSend!=NULL) {
      openserial_printError(COMPONENT_TCP,ERR_BUSY_SENDING,(errorparameter_t)0,(errorparameter_t)0);
      return E_FAIL;
   }
   //I receive command 'send', I send data
   msg->l4_protocol          = IANA_TCP;
   msg->l4_sourcePortORicmpv6Type       = tcp_myPort;
   msg->l4_destination_port  = tcp_hisPort;
   msg->l4_payload           = msg->payload;
   msg->l4_length            = msg->length;
   memcpy(&(msg->l3_destinationORsource),&tcp_hisIPv6Address,sizeof(open_addr_t));
   tcp_dataToSend = msg;
   prependTCPHeader(tcp_dataToSend,
         TCP_ACK_YES,
         TCP_PSH_YES,
         TCP_RST_NO,
         TCP_SYN_NO,
         TCP_FIN_NO);
   tcp_mySeqNum += tcp_dataToSend->l4_length;
   tcp_change_state(TCP_STATE_ALMOST_DATA_SENT);
   return forwarding_send(tcp_dataToSend);
}

void tcp_sendDone(OpenQueueEntry_t* msg, error_t error) {
   OpenQueueEntry_t* tempPkt;
   msg->owner = COMPONENT_TCP;
   switch (tcp_state) {
      case TCP_STATE_ALMOST_SYN_SENT:                             //[sendDone] establishement
         openqueue_freePacketBuffer(msg);
         tcp_change_state(TCP_STATE_SYN_SENT);
         break;

      case TCP_STATE_ALMOST_SYN_RECEIVED:                         //[sendDone] establishement
         openqueue_freePacketBuffer(msg);
         tcp_change_state(TCP_STATE_SYN_RECEIVED);
         break;

      case TCP_STATE_ALMOST_ESTABLISHED:                          //[sendDone] establishement
         openqueue_freePacketBuffer(msg);
         tcp_change_state(TCP_STATE_ESTABLISHED);
         switch(tcp_myPort) {
            case WKP_TCP_ECHO:
               apptcpecho_connectDone(E_SUCCESS);
               break;
            case WKP_TCP_INJECT:
               apptcpinject_connectDone(E_SUCCESS);
               break;   
            case WKP_TCP_HTTP:
               apptcpohlone_connectDone(E_SUCCESS);
               break;
            case WKP_TCP_DISCARD:
               apptcpprint_connectDone(E_SUCCESS);
               break;
            default:
               openserial_printError(COMPONENT_TCP,ERR_UNSUPPORTED_PORT_NUMBER,tcp_myPort,2);
               break;
         }
         break;

      case TCP_STATE_ALMOST_DATA_SENT:                            //[sendDone] data
         tcp_change_state(TCP_STATE_DATA_SENT);
         break;

      case TCP_STATE_ALMOST_DATA_RECEIVED:                        //[sendDone] data
         openqueue_freePacketBuffer(msg);
         tcp_change_state(TCP_STATE_ESTABLISHED);
         switch(tcp_myPort) {
            case WKP_TCP_ECHO:
               apptcpecho_receive(tcp_dataReceived);
               break;
            case WKP_TCP_INJECT:
               apptcpinject_receive(tcp_dataReceived);
               break;
            case WKP_TCP_HTTP:
               apptcpohlone_receive(tcp_dataReceived);
               break;
            case WKP_TCP_DISCARD:
               apptcpprint_receive(tcp_dataReceived);
               break;
            default:
               openserial_printError(COMPONENT_TCP,ERR_UNSUPPORTED_PORT_NUMBER,tcp_myPort,0);
               openqueue_freePacketBuffer(msg);
               tcp_dataReceived = NULL;
               break;
         }
         break;

      case TCP_STATE_ALMOST_FIN_WAIT_1:                           //[sendDone] teardown
         openqueue_freePacketBuffer(msg);
         tcp_change_state(TCP_STATE_FIN_WAIT_1);
         break;

      case TCP_STATE_ALMOST_CLOSING:                              //[sendDone] teardown
         openqueue_freePacketBuffer(msg);
         tcp_change_state(TCP_STATE_CLOSING);
         break;

      case TCP_STATE_ALMOST_TIME_WAIT:                            //[sendDone] teardown
         openqueue_freePacketBuffer(msg);
         tcp_change_state(TCP_STATE_TIME_WAIT);
         //TODO implement waiting timer
         reset();
         break;

      case TCP_STATE_ALMOST_CLOSE_WAIT:                           //[sendDone] teardown
         openqueue_freePacketBuffer(msg);
         tcp_change_state(TCP_STATE_CLOSE_WAIT);
         //I send FIN+ACK
         tempPkt = openqueue_getFreePacketBuffer();
         if (tempPkt==NULL) {
            openserial_printError(COMPONENT_TCP,ERR_NO_FREE_PACKET_BUFFER,0,0);
            openqueue_freePacketBuffer(msg);
            return;
         }
         tempPkt->creator       = COMPONENT_TCP;
         tempPkt->owner         = COMPONENT_TCP;
         memcpy(&(tempPkt->l3_destinationORsource),&tcp_hisIPv6Address,sizeof(open_addr_t));
         prependTCPHeader(tempPkt,
               TCP_ACK_YES,
               TCP_PSH_NO,
               TCP_RST_NO,
               TCP_SYN_NO,
               TCP_FIN_YES);
         forwarding_send(tempPkt);
         tcp_change_state(TCP_STATE_ALMOST_LAST_ACK);
         break;

      case TCP_STATE_ALMOST_LAST_ACK:                             //[sendDone] teardown
         openqueue_freePacketBuffer(msg);
         tcp_change_state(TCP_STATE_LAST_ACK);
         break;

      default:
         openserial_printError(COMPONENT_TCP,ERR_WRONG_TCP_STATE,(errorparameter_t)tcp_state,(errorparameter_t)3);
         break;
   }
}

void tcp_receive(OpenQueueEntry_t* msg) {
   OpenQueueEntry_t* tempPkt;
   bool shouldIlisten;
   msg->owner                     = COMPONENT_TCP;
   msg->l4_protocol               = IANA_TCP;
   msg->l4_payload                = msg->payload;
   msg->l4_length                 = msg->length;
   msg->l4_sourcePortORicmpv6Type = packetfunctions_ntohs((uint8_t*)&(((tcp_ht*)msg->payload)->source_port));
   msg->l4_destination_port       = packetfunctions_ntohs((uint8_t*)&(((tcp_ht*)msg->payload)->destination_port));
   if ( 
         tcp_state!=TCP_STATE_CLOSED &&
         (
          msg->l4_destination_port != tcp_myPort  ||
          msg->l4_sourcePortORicmpv6Type      != tcp_hisPort ||
          packetfunctions_sameAddress(&(msg->l3_destinationORsource),&tcp_hisIPv6Address)==FALSE
         )
      ) {
      openqueue_freePacketBuffer(msg);
      return;
   }
   if (containsControlBits(msg,TCP_ACK_WHATEVER,TCP_RST_YES,TCP_SYN_WHATEVER,TCP_FIN_WHATEVER)) {
      //I receive RST[+*], I reset
      reset();
      openqueue_freePacketBuffer(msg);
   }
   switch (tcp_state) {
      case TCP_STATE_CLOSED:                                      //[receive] establishement
         switch(msg->l4_destination_port) {
            case WKP_TCP_ECHO:
               shouldIlisten = apptcpecho_shouldIlisten();
               break;
            case WKP_TCP_INJECT:
               shouldIlisten = apptcpinject_shouldIlisten();
               break;   
            case WKP_TCP_HTTP:
               shouldIlisten = apptcpohlone_shouldIlisten();
               break;
            case WKP_TCP_DISCARD:
               shouldIlisten = apptcpprint_shouldIlisten();
               break;
            default:
               openserial_printError(COMPONENT_TCP,ERR_UNSUPPORTED_PORT_NUMBER,msg->l4_sourcePortORicmpv6Type,3);
               shouldIlisten = FALSE;
               break;
         }
         if ( containsControlBits(msg,TCP_ACK_NO,TCP_RST_NO,TCP_SYN_YES,TCP_FIN_NO) && shouldIlisten==TRUE ) {
                  tcp_myPort = msg->l4_destination_port;
                  //I receive SYN, I send SYN+ACK
                  tcp_hisNextSeqNum = (packetfunctions_ntohl((uint8_t*)&(((tcp_ht*)msg->payload)->sequence_number)))+1;
                  tcp_hisPort       = msg->l4_sourcePortORicmpv6Type;
                  memcpy(&tcp_hisIPv6Address,&(msg->l3_destinationORsource),sizeof(open_addr_t));
                  tempPkt       = openqueue_getFreePacketBuffer();
                  if (tempPkt==NULL) {
                     openserial_printError(COMPONENT_TCP,ERR_NO_FREE_PACKET_BUFFER,0,0);
                     openqueue_freePacketBuffer(msg);
                     return;
                  }
                  tempPkt->creator       = COMPONENT_TCP;
                  tempPkt->owner         = COMPONENT_TCP;
                  memcpy(&(tempPkt->l3_destinationORsource),&tcp_hisIPv6Address,sizeof(open_addr_t));
                  prependTCPHeader(tempPkt,
                        TCP_ACK_YES,
                        TCP_PSH_NO,
                        TCP_RST_NO,
                        TCP_SYN_YES,
                        TCP_FIN_NO);
                  tcp_mySeqNum++;
                  tcp_change_state(TCP_STATE_ALMOST_SYN_RECEIVED);
                  forwarding_send(tempPkt);
               } else {
                  reset();
                  openserial_printError(COMPONENT_TCP,ERR_RESET,(errorparameter_t)tcp_state,(errorparameter_t)0);
               }
         openqueue_freePacketBuffer(msg);
         break;

      case TCP_STATE_SYN_SENT:                                    //[receive] establishement
         if (containsControlBits(msg,TCP_ACK_YES,TCP_RST_NO,TCP_SYN_YES,TCP_FIN_NO)) {
            //I receive SYN+ACK, I send ACK
            tcp_hisNextSeqNum = (packetfunctions_ntohl((uint8_t*)&(((tcp_ht*)msg->payload)->sequence_number)))+1;
            tempPkt = openqueue_getFreePacketBuffer();
            if (tempPkt==NULL) {
               openserial_printError(COMPONENT_TCP,ERR_NO_FREE_PACKET_BUFFER,0,0);
               openqueue_freePacketBuffer(msg);
               return;
            }
            tempPkt->creator       = COMPONENT_TCP;
            tempPkt->owner         = COMPONENT_TCP;
            memcpy(&(tempPkt->l3_destinationORsource),&tcp_hisIPv6Address,sizeof(open_addr_t));
            prependTCPHeader(tempPkt,
                  TCP_ACK_YES,
                  TCP_PSH_NO,
                  TCP_RST_NO,
                  TCP_SYN_NO,
                  TCP_FIN_NO);
            tcp_change_state(TCP_STATE_ALMOST_ESTABLISHED);
            forwarding_send(tempPkt);
         } else if (containsControlBits(msg,TCP_ACK_NO,TCP_RST_NO,TCP_SYN_YES,TCP_FIN_NO)) {
            //I receive SYN, I send SYN+ACK
            tcp_hisNextSeqNum = (packetfunctions_ntohl((uint8_t*)&(((tcp_ht*)msg->payload)->sequence_number)))+1;
            tempPkt       = openqueue_getFreePacketBuffer();
            if (tempPkt==NULL) {
               openserial_printError(COMPONENT_TCP,ERR_NO_FREE_PACKET_BUFFER,0,0);
               openqueue_freePacketBuffer(msg);
               return;
            }
            tempPkt->creator       = COMPONENT_TCP;
            tempPkt->owner         = COMPONENT_TCP;
            memcpy(&(tempPkt->l3_destinationORsource),&tcp_hisIPv6Address,sizeof(open_addr_t));
            prependTCPHeader(tempPkt,
                  TCP_ACK_YES,
                  TCP_PSH_NO,
                  TCP_RST_NO,
                  TCP_SYN_YES,
                  TCP_FIN_NO);
            tcp_mySeqNum++;
            tcp_change_state(TCP_STATE_ALMOST_SYN_RECEIVED);
            forwarding_send(tempPkt);
         } else {
            reset();
            openserial_printError(COMPONENT_TCP,ERR_RESET,(errorparameter_t)tcp_state,(errorparameter_t)1);
         }
         openqueue_freePacketBuffer(msg);
         break;

      case TCP_STATE_SYN_RECEIVED:                                //[receive] establishement
         if (containsControlBits(msg,TCP_ACK_YES,TCP_RST_NO,TCP_SYN_NO,TCP_FIN_NO)) {
            //I receive ACK, the virtual circuit is established
            tcp_change_state(TCP_STATE_ESTABLISHED);
         } else {
            reset();
            openserial_printError(COMPONENT_TCP,ERR_RESET,(errorparameter_t)tcp_state,(errorparameter_t)2);
         }
         openqueue_freePacketBuffer(msg);
         break;

      case TCP_STATE_ESTABLISHED:                                 //[receive] data/teardown
         if (containsControlBits(msg,TCP_ACK_WHATEVER,TCP_RST_NO,TCP_SYN_NO,TCP_FIN_YES)) {
            //I receive FIN[+ACK], I send ACK
            tcp_hisNextSeqNum = (packetfunctions_ntohl((uint8_t*)&(((tcp_ht*)msg->payload)->sequence_number)))+msg->length-sizeof(tcp_ht)+1;
            tempPkt = openqueue_getFreePacketBuffer();
            if (tempPkt==NULL) {
               openserial_printError(COMPONENT_TCP,ERR_NO_FREE_PACKET_BUFFER,0,0);
               openqueue_freePacketBuffer(msg);
               return;
            }
            tempPkt->creator       = COMPONENT_TCP;
            tempPkt->owner         = COMPONENT_TCP;
            memcpy(&(tempPkt->l3_destinationORsource),&tcp_hisIPv6Address,sizeof(open_addr_t));
            prependTCPHeader(tempPkt,
                  TCP_ACK_YES,
                  TCP_PSH_NO,
                  TCP_RST_NO,
                  TCP_SYN_NO,
                  TCP_FIN_NO);
            forwarding_send(tempPkt);
            tcp_change_state(TCP_STATE_ALMOST_CLOSE_WAIT);
         } else if (containsControlBits(msg,TCP_ACK_WHATEVER,TCP_RST_NO,TCP_SYN_NO,TCP_FIN_NO)) {
            //I receive data, I send ACK
            tcp_hisNextSeqNum = (packetfunctions_ntohl((uint8_t*)&(((tcp_ht*)msg->payload)->sequence_number)))+msg->length-sizeof(tcp_ht);
            tempPkt = openqueue_getFreePacketBuffer();
            if (tempPkt==NULL) {
               openserial_printError(COMPONENT_TCP,ERR_NO_FREE_PACKET_BUFFER,0,0);
               openqueue_freePacketBuffer(msg);
               return;
            }
            tempPkt->creator       = COMPONENT_TCP;
            tempPkt->owner         = COMPONENT_TCP;
            memcpy(&(tempPkt->l3_destinationORsource),&tcp_hisIPv6Address,sizeof(open_addr_t));
            prependTCPHeader(tempPkt,
                  TCP_ACK_YES,
                  TCP_PSH_NO,
                  TCP_RST_NO,
                  TCP_SYN_NO,
                  TCP_FIN_NO);
            forwarding_send(tempPkt);
            packetfunctions_tossHeader(msg,sizeof(tcp_ht));
            tcp_dataReceived = msg;
            tcp_change_state(TCP_STATE_ALMOST_DATA_RECEIVED);
         } else {
            reset();
            openserial_printError(COMPONENT_TCP,ERR_RESET,(errorparameter_t)tcp_state,(errorparameter_t)3);
            openqueue_freePacketBuffer(msg);
         }
         break;

      case TCP_STATE_DATA_SENT:                                   //[receive] data
         if (containsControlBits(msg,TCP_ACK_YES,TCP_RST_NO,TCP_SYN_NO,TCP_FIN_NO)) {
            //I receive ACK, data message sent
            switch(tcp_myPort) {
               case WKP_TCP_ECHO:
                  apptcpecho_sendDone(tcp_dataToSend,E_SUCCESS);
                  break;
               case WKP_TCP_INJECT:
                  apptcpinject_sendDone(tcp_dataToSend,E_SUCCESS);
                  break;
               case WKP_TCP_HTTP:
                  apptcpohlone_sendDone(tcp_dataToSend,E_SUCCESS);
                  break;
               case WKP_TCP_DISCARD:
                  apptcpprint_sendDone(tcp_dataToSend,E_SUCCESS);
                  break;
               default:
                  openserial_printError(COMPONENT_TCP,ERR_UNSUPPORTED_PORT_NUMBER,tcp_myPort,0);
                  break;
            }
            tcp_dataToSend = NULL;
            tcp_change_state(TCP_STATE_ESTABLISHED);
         } else if (containsControlBits(msg,TCP_ACK_WHATEVER,TCP_RST_NO,TCP_SYN_NO,TCP_FIN_YES)) {
            //I receive FIN[+ACK], I send ACK
            switch(tcp_myPort) {
               case WKP_TCP_ECHO:
                  apptcpecho_sendDone(tcp_dataToSend,E_SUCCESS);
                  break;
               case WKP_TCP_INJECT:
                  apptcpinject_sendDone(tcp_dataToSend,E_SUCCESS);
                  break;
               case WKP_TCP_HTTP:
                  apptcpohlone_sendDone(tcp_dataToSend,E_SUCCESS);
                  break;
               case WKP_TCP_DISCARD:
                  apptcpprint_sendDone(tcp_dataToSend,E_SUCCESS);
                  break;
               default:
                  openserial_printError(COMPONENT_TCP,ERR_UNSUPPORTED_PORT_NUMBER,tcp_myPort,0);
                  break;
            }
            tcp_dataToSend = NULL;
            tcp_hisNextSeqNum = (packetfunctions_ntohl((uint8_t*)&(((tcp_ht*)msg->payload)->sequence_number)))+msg->length-sizeof(tcp_ht)+1;
            tempPkt = openqueue_getFreePacketBuffer();
            if (tempPkt==NULL) {
               openserial_printError(COMPONENT_TCP,ERR_NO_FREE_PACKET_BUFFER,0,0);
               openqueue_freePacketBuffer(msg);
               return;
            }
            tempPkt->creator       = COMPONENT_TCP;
            tempPkt->owner         = COMPONENT_TCP;
            memcpy(&(tempPkt->l3_destinationORsource),&tcp_hisIPv6Address,sizeof(open_addr_t));
            prependTCPHeader(tempPkt,
                  TCP_ACK_YES,
                  TCP_PSH_NO,
                  TCP_RST_NO,
                  TCP_SYN_NO,
                  TCP_FIN_NO);
            forwarding_send(tempPkt);
            tcp_change_state(TCP_STATE_ALMOST_CLOSE_WAIT);
         } else {
            reset();
            openserial_printError(COMPONENT_TCP,ERR_RESET,(errorparameter_t)tcp_state,(errorparameter_t)4);
         }
         openqueue_freePacketBuffer(msg);
         break;

      case TCP_STATE_FIN_WAIT_1:                                  //[receive] teardown
         if (containsControlBits(msg,TCP_ACK_NO,TCP_RST_NO,TCP_SYN_NO,TCP_FIN_YES)) {
            //I receive FIN, I send ACK
            tcp_hisNextSeqNum = (packetfunctions_ntohl((uint8_t*)&(((tcp_ht*)msg->payload)->sequence_number)))+1;
            tempPkt = openqueue_getFreePacketBuffer();
            if (tempPkt==NULL) {
               openserial_printError(COMPONENT_TCP,ERR_NO_FREE_PACKET_BUFFER,0,0);
               openqueue_freePacketBuffer(msg);
               return;
            }
            tempPkt->creator       = COMPONENT_TCP;
            tempPkt->owner         = COMPONENT_TCP;
            memcpy(&(tempPkt->l3_destinationORsource),&tcp_hisIPv6Address,sizeof(open_addr_t));
            prependTCPHeader(tempPkt,
                  TCP_ACK_YES,
                  TCP_PSH_NO,
                  TCP_RST_NO,
                  TCP_SYN_NO,
                  TCP_FIN_NO);
            forwarding_send(tempPkt);
            tcp_change_state(TCP_STATE_ALMOST_CLOSING);
         } else if (containsControlBits(msg,TCP_ACK_YES,TCP_RST_NO,TCP_SYN_NO,TCP_FIN_YES)) {
            //I receive FIN+ACK, I send ACK
            tcp_hisNextSeqNum = (packetfunctions_ntohl((uint8_t*)&(((tcp_ht*)msg->payload)->sequence_number)))+1;
            tempPkt = openqueue_getFreePacketBuffer();
            if (tempPkt==NULL) {
               openserial_printError(COMPONENT_TCP,ERR_NO_FREE_PACKET_BUFFER,0,0);
               openqueue_freePacketBuffer(msg);
               return;
            }
            tempPkt->creator       = COMPONENT_TCP;
            tempPkt->owner         = COMPONENT_TCP;
            memcpy(&(tempPkt->l3_destinationORsource),&tcp_hisIPv6Address,sizeof(open_addr_t));
            prependTCPHeader(tempPkt,
                  TCP_ACK_YES,
                  TCP_PSH_NO,
                  TCP_RST_NO,
                  TCP_SYN_NO,
                  TCP_FIN_NO);
            forwarding_send(tempPkt);
            tcp_change_state(TCP_STATE_ALMOST_TIME_WAIT);
         } else if  (containsControlBits(msg,TCP_ACK_YES,TCP_RST_NO,TCP_SYN_NO,TCP_FIN_NO)) {
            //I receive ACK, I will receive FIN later
            tcp_change_state(TCP_STATE_FIN_WAIT_2);
         } else {
            reset();
            openserial_printError(COMPONENT_TCP,ERR_RESET,(errorparameter_t)tcp_state,(errorparameter_t)5);
         }
         openqueue_freePacketBuffer(msg);
         break;

      case TCP_STATE_FIN_WAIT_2:                                  //[receive] teardown
         if (containsControlBits(msg,TCP_ACK_WHATEVER,TCP_RST_NO,TCP_SYN_NO,TCP_FIN_YES)) {
            //I receive FIN[+ACK], I send ACK
            tcp_hisNextSeqNum = (packetfunctions_ntohl((uint8_t*)&(((tcp_ht*)msg->payload)->sequence_number)))+1;
            tempPkt = openqueue_getFreePacketBuffer();
            if (tempPkt==NULL) {
               openserial_printError(COMPONENT_TCP,ERR_NO_FREE_PACKET_BUFFER,0,0);
               openqueue_freePacketBuffer(msg);
               return;
            }
            tempPkt->creator       = COMPONENT_TCP;
            tempPkt->owner         = COMPONENT_TCP;
            memcpy(&(tempPkt->l3_destinationORsource),&tcp_hisIPv6Address,sizeof(open_addr_t));
            prependTCPHeader(tempPkt,
                  TCP_ACK_YES,
                  TCP_PSH_NO,
                  TCP_RST_NO,
                  TCP_SYN_NO,
                  TCP_FIN_NO);
            forwarding_send(tempPkt);
            tcp_change_state(TCP_STATE_ALMOST_TIME_WAIT);
         }
         openqueue_freePacketBuffer(msg);
         break;

      case TCP_STATE_CLOSING:                                     //[receive] teardown
         if (containsControlBits(msg,TCP_ACK_YES,TCP_RST_NO,TCP_SYN_NO,TCP_FIN_NO)) {
            //I receive ACK, I do nothing
            tcp_change_state(TCP_STATE_TIME_WAIT);
            //TODO implement waiting timer
            reset();
         }
         openqueue_freePacketBuffer(msg);
         break;

      case TCP_STATE_LAST_ACK:                                    //[receive] teardown
         if (containsControlBits(msg,TCP_ACK_YES,TCP_RST_NO,TCP_SYN_NO,TCP_FIN_NO)) {
            //I receive ACK, I reset
            reset();
         }
         openqueue_freePacketBuffer(msg);
         break;

      default:
         openserial_printError(COMPONENT_TCP,ERR_WRONG_TCP_STATE,(errorparameter_t)tcp_state,(errorparameter_t)4);
         break;
   }
}

error_t tcp_close() {    //[command] teardown
   OpenQueueEntry_t* tempPkt;
   if (  tcp_state==TCP_STATE_ALMOST_CLOSE_WAIT ||
         tcp_state==TCP_STATE_CLOSE_WAIT        ||
         tcp_state==TCP_STATE_ALMOST_LAST_ACK   ||
         tcp_state==TCP_STATE_LAST_ACK          ||
         tcp_state==TCP_STATE_CLOSED) {
      //openserial_printError(COMPONENT_TCP,ERR_WRONG_TCP_STATE,(errorparameter_t)tcp_state,(errorparameter_t)1);
      //not an error, can happen when distant node has already started tearing down
      return E_SUCCESS;
   }
   //I receive command 'close', I send FIN+ACK
   tempPkt = openqueue_getFreePacketBuffer();
   if (tempPkt==NULL) {
      openserial_printError(COMPONENT_TCP,ERR_NO_FREE_PACKET_BUFFER,0,0);
      return E_FAIL;
   }
   tempPkt->creator       = COMPONENT_TCP;
   tempPkt->owner         = COMPONENT_TCP;
   memcpy(&(tempPkt->l3_destinationORsource),&tcp_hisIPv6Address,sizeof(open_addr_t));
   prependTCPHeader(tempPkt,
         TCP_ACK_YES,
         TCP_PSH_NO,
         TCP_RST_NO,
         TCP_SYN_NO,
         TCP_FIN_YES);
   tcp_mySeqNum++;
   tcp_change_state(TCP_STATE_ALMOST_FIN_WAIT_1);
   return forwarding_send(tempPkt);
}

bool tcp_debugPrint() {
   return FALSE;
}

//======= timer

//timer used to reset state when TCP state machine is stuck
void timer_tcp_timeout_fired() {
   reset();
}

//=========================== private =========================================

void prependTCPHeader(OpenQueueEntry_t* msg,
      bool ack,
      bool push,
      bool rst,
      bool syn,
      bool fin) {
   msg->l4_protocol = IANA_TCP;
   packetfunctions_reserveHeaderSize(msg,sizeof(tcp_ht));
   packetfunctions_htons(tcp_myPort        ,(uint8_t*)&(((tcp_ht*)msg->payload)->source_port));
   packetfunctions_htons(tcp_hisPort       ,(uint8_t*)&(((tcp_ht*)msg->payload)->destination_port));
   packetfunctions_htonl(tcp_mySeqNum      ,(uint8_t*)&(((tcp_ht*)msg->payload)->sequence_number));
   packetfunctions_htonl(tcp_hisNextSeqNum ,(uint8_t*)&(((tcp_ht*)msg->payload)->ack_number));
   ((tcp_ht*)msg->payload)->data_offset      = TCP_DEFAULT_DATA_OFFSET;
   ((tcp_ht*)msg->payload)->control_bits     = 0;
   if (ack==TCP_ACK_YES) {
      ((tcp_ht*)msg->payload)->control_bits |= 1 << TCP_ACK;
   } else {
      packetfunctions_htonl(0,(uint8_t*)&(((tcp_ht*)msg->payload)->ack_number));
   }
   if (push==TCP_PSH_YES) {
      ((tcp_ht*)msg->payload)->control_bits |= 1 << TCP_PSH;
   }
   if (rst==TCP_RST_YES) {
      ((tcp_ht*)msg->payload)->control_bits |= 1 << TCP_RST;
   }
   if (syn==TCP_SYN_YES) {
      ((tcp_ht*)msg->payload)->control_bits |= 1 << TCP_SYN;
   }
   if (fin==TCP_FIN_YES) {
      ((tcp_ht*)msg->payload)->control_bits |= 1 << TCP_FIN;
   }
   packetfunctions_htons(TCP_DEFAULT_WINDOW_SIZE    ,(uint8_t*)&(((tcp_ht*)msg->payload)->window_size));
   packetfunctions_htons(TCP_DEFAULT_URGENT_POINTER ,(uint8_t*)&(((tcp_ht*)msg->payload)->urgent_pointer));
   //calculate checksum last to take all header fields into account
   packetfunctions_calculateChecksum(msg,(uint8_t*)&(((tcp_ht*)msg->payload)->checksum));
}

bool containsControlBits(OpenQueueEntry_t* msg, uint8_t ack, uint8_t rst, uint8_t syn, uint8_t fin) {
   bool return_value = TRUE;
   if (ack!=TCP_ACK_WHATEVER){
      return_value = return_value && ((bool)( (((tcp_ht*)msg->payload)->control_bits >> TCP_ACK) & 0x01) == ack);
   }
   if (rst!=TCP_RST_WHATEVER){
      return_value = return_value && ((bool)( (((tcp_ht*)msg->payload)->control_bits >> TCP_RST) & 0x01) == rst);
   }
   if (syn!=TCP_SYN_WHATEVER){
      return_value = return_value && ((bool)( (((tcp_ht*)msg->payload)->control_bits >> TCP_SYN) & 0x01) == syn);
   }
   if (fin!=TCP_FIN_WHATEVER){
      return_value = return_value && ((bool)( (((tcp_ht*)msg->payload)->control_bits >> TCP_FIN) & 0x01) == fin);
   }
   return return_value;
}

void reset() {
   tcp_change_state(TCP_STATE_CLOSED);
   tcp_mySeqNum            = TCP_INITIAL_SEQNUM; 
   tcp_hisNextSeqNum       = 0;
   tcp_hisPort             = 0;
   tcp_hisIPv6Address.type = ADDR_NONE;
   tcp_dataToSend          = NULL;
   tcp_dataReceived        = NULL;
   openqueue_removeAllOwnedBy(COMPONENT_TCP);
}

void tcp_change_state(uint8_t new_tcp_state) {
   tcp_state = new_tcp_state;
   if (tcp_state==TCP_STATE_CLOSED) {
      timer_stop(TIMER_TCP_TIMEOUT);
   } else {
      timer_startOneShot(TIMER_TCP_TIMEOUT,TCP_TIMEOUT);
   }
}