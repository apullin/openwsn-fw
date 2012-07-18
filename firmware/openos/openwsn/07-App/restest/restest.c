#include "openwsn.h"
#include "restest.h"
#include "opencoap.h"
#include "opentimers.h"
#include "openqueue.h"
#include "packetfunctions.h"
#include "openserial.h"
#include "openrandom.h"
#include "scheduler.h"
#include "IEEE802154E.h"
#include "reservation.h"
#include "packetfunctions.h"

//=========================== defines =========================================

#define PAYLOADLEN     80

const uint8_t restest_path0[] = "res"; // debug/restest

//=========================== variables =======================================

typedef struct {
   open_addr_t        neighbor16bID;
   uint16_t           request_time;//time in ms
   uint8_t            num_of_links;
   bool               isRequestPending;
} request_vars_t;

typedef struct {
   coap_resource_desc_t desc;
   uint8_t num_requests;
   uint8_t num_success; //num_failures = num_requests - num_success
   asn_t req_init;//time when the reservation request was initiated
   request_vars_t request_vars;
} restest_vars_t;

restest_vars_t restest_vars;

PRAGMA(pack(1));
typedef struct {
   bool reservation_status;
   uint16_t service_time;
   uint8_t numRequests;
   uint8_t numSuccess;
}restest_payload_t;
PRAGMA(pack());

restest_payload_t restest_payload;

//=========================== prototypes ======================================

error_t restest_receive(OpenQueueEntry_t* msg,
                    coap_header_iht*  coap_header,
                    coap_option_iht*  coap_options);
void    restest_timer_cb();
void    restest_sendDone(OpenQueueEntry_t* msg,
                       error_t error);
void restest_reservation_granted_cb();
void restest_reservation_failed_cb();
//=========================== public ==========================================

void restest_init() {
  
   restest_vars.num_requests =0;
   restest_vars.num_success=0;
   restest_vars.request_vars.isRequestPending=FALSE;//no requests pending
  
  // prepare the resource descriptor for the /restest path
   restest_vars.desc.path0len             = sizeof(restest_path0)-1;
   restest_vars.desc.path0val             = (uint8_t*)(&restest_path0);
   restest_vars.desc.path1len             = 0;
   restest_vars.desc.path1val             = NULL;
   restest_vars.desc.componentID          = COMPONENT_RESTEST;
   restest_vars.desc.callbackRx           = &restest_receive;
   restest_vars.desc.callbackSendDone     = &restest_sendDone;
   
   //register the callbacks from l2.5
   reservation_setcb(restest_reservation_granted_cb, restest_reservation_failed_cb);
   
   opencoap_register(&restest_vars.desc);
}

//=========================== private =========================================

error_t restest_receive(OpenQueueEntry_t* msg,
                      coap_header_iht* coap_header,
                      coap_option_iht* coap_options) {
   return E_FAIL;
}

void restest_serial_trigger(){
  //first get the serial input buffer:
   uint8_t           msg[136];//worst case: 8B of next hop + 128B of data
   uint8_t           numDataBytes;

   numDataBytes = openserial_getNumDataBytes();
   openserial_getInputBuffer(&(msg[0]),numDataBytes);
  
   //copy request parameters (this needs review of course):
   restest_vars.request_vars.neighbor16bID.addr_64b[7] = msg[1];
   restest_vars.request_vars.num_of_links = msg[2];
   // store the request time
   restest_vars.request_vars.request_time   =     (msg[3]<<8)&0xff00 + msg[4]&0x00ff;
   
   restest_vars.request_vars.isRequestPending = TRUE;
   //start one-shot timer with asn difference
   opentimers_start(restest_vars.request_vars.request_time,TIMER_ONESHOT,TIME_MS,restest_timer_cb);
}

void restest_timer_cb(){
  //in this function, request a bunch of links from layer 2.5
   ieee154e_get_asn(&restest_vars.req_init);
   reservation_LinkRequest(&restest_vars.request_vars.neighbor16bID,restest_vars.request_vars.num_of_links);
}

void restest_reservation_granted_cb() {
   uint8_t numOptions;
   error_t outcome;
   OpenQueueEntry_t* pkt;
   pkt = openqueue_getFreePacketBuffer(COMPONENT_RESTEST);
   if (pkt==NULL) {
      openserial_printError(COMPONENT_RESTEST,ERR_NO_FREE_PACKET_BUFFER,
                            (errorparameter_t)0,
                            (errorparameter_t)0);
      openqueue_freePacketBuffer(pkt);
      return;
   }
   
   // take ownership over that packet
   pkt->creator    = COMPONENT_RESTEST;
   pkt->owner      = COMPONENT_RESTEST;
   
   //reservation succeeded
   packetfunctions_reserveHeaderSize(pkt,sizeof(restest_payload)-1);
   restest_payload.reservation_status = TRUE;
   restest_payload.service_time = 15*ieee154e_asnDiff(&(restest_vars.req_init));
   restest_payload.numRequests = ++restest_vars.num_requests;
   restest_payload.numSuccess = ++restest_vars.num_success;
   memcpy(&pkt->payload[0],&restest_payload,sizeof(restest_payload)-1);
   
   //continue coap packet
   numOptions = 0;
   // location-path option
   packetfunctions_reserveHeaderSize(pkt,sizeof(restest_path0)-1);
   memcpy(&pkt->payload[0],&restest_path0,sizeof(restest_path0)-1);
   packetfunctions_reserveHeaderSize(pkt,1);
   pkt->payload[0]                  = (COAP_OPTION_LOCATIONPATH-COAP_OPTION_CONTENTTYPE) << 4 |
      sizeof(restest_path0)-1;
   numOptions++;
   // content-type option
   packetfunctions_reserveHeaderSize(pkt,2);
   pkt->payload[0]                  = COAP_OPTION_CONTENTTYPE << 4 |
      1;
   pkt->payload[1]                  = COAP_MEDTYPE_APPOCTETSTREAM;
   numOptions++;
   // metadata
   pkt->l4_destination_port         = WKP_UDP_COAP;
   pkt->l3_destinationORsource.type = ADDR_128B;
   memcpy(&pkt->l3_destinationORsource.addr_128b[0],&ipAddr_local,16);
   // send
   outcome = opencoap_send(pkt,
                           COAP_TYPE_NON,
                           COAP_CODE_REQ_PUT,
                           numOptions,
                           &restest_vars.desc);
   // avoid overflowing the queue if fails
   if (outcome==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
   }
   
   return;
}

void restest_reservation_failed_cb(){
   uint8_t numOptions;
   error_t outcome;
   OpenQueueEntry_t* pkt;
   pkt = openqueue_getFreePacketBuffer(COMPONENT_RESTEST);
   if (pkt==NULL) {
      openserial_printError(COMPONENT_RESTEST,ERR_NO_FREE_PACKET_BUFFER,
                            (errorparameter_t)0,
                            (errorparameter_t)0);
      openqueue_freePacketBuffer(pkt);
      return;
   }
   
   // take ownership over that packet
   pkt->creator    = COMPONENT_RESTEST;
   pkt->owner      = COMPONENT_RESTEST;
   
   //reservation failed
   packetfunctions_reserveHeaderSize(pkt,sizeof(restest_payload)-1);
   restest_payload.reservation_status = FALSE;
   restest_payload.service_time = 15*ieee154e_asnDiff(&(restest_vars.req_init));
   restest_payload.numRequests = ++restest_vars.num_requests;
   restest_payload.numSuccess = restest_vars.num_success;
   memcpy(&pkt->payload[0],&restest_payload,sizeof(restest_payload)-1);
   
   //continue coap packet
   numOptions = 0;
   // location-path option
   packetfunctions_reserveHeaderSize(pkt,sizeof(restest_path0)-1);
   memcpy(&pkt->payload[0],&restest_path0,sizeof(restest_path0)-1);
   packetfunctions_reserveHeaderSize(pkt,1);
   pkt->payload[0]                  = (COAP_OPTION_LOCATIONPATH-COAP_OPTION_CONTENTTYPE) << 4 |
      sizeof(restest_path0)-1;
   numOptions++;
   // content-type option
   packetfunctions_reserveHeaderSize(pkt,2);
   pkt->payload[0]                  = COAP_OPTION_CONTENTTYPE << 4 |
      1;
   pkt->payload[1]                  = COAP_MEDTYPE_APPOCTETSTREAM;
   numOptions++;
   // metadata
   pkt->l4_destination_port         = WKP_UDP_COAP;
   pkt->l3_destinationORsource.type = ADDR_128B;
   memcpy(&pkt->l3_destinationORsource.addr_128b[0],&ipAddr_local,16);
   // send
   outcome = opencoap_send(pkt,
                           COAP_TYPE_NON,
                           COAP_CODE_REQ_PUT,
                           numOptions,
                           &restest_vars.desc);
   // avoid overflowing the queue if fails
   if (outcome==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
   }
   
   return;
  
}

void restest_sendDone(OpenQueueEntry_t* msg, error_t error) {
   openqueue_freePacketBuffer(msg);
}
