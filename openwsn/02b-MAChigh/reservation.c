#include "openwsn.h"
#include "res.h"
#include "idmanager.h"
#include "openserial.h"
#include "openqueue.h"
#include "neighbors.h"
#include "iphc.h"
#include "leds.h"
#include "packetfunctions.h"
#include "openrandom.h"
#include "schedule.h"
#include "scheduler.h"
#include "bsp_timer.h"
#include "opentimers.h"
#include "processIE.h"
#include "IEfield.h"
#include "reservation.h"


//=========================== typedef =========================================

//=========================== variables =======================================

typedef struct {
  uint8_t commandID;
  bandwidth_vars_t bandwidth_vars;
} reservation_vars_t;

reservation_vars_t reservation_vars;
//=========================== prototypes ======================================

//admin
void    reservation_init(){
  memset(&reservation_vars,0,sizeof(reservation_vars_t));
}
// public
uint8_t reservation_getuResCommandID(){
  return reservation_vars.commandID;
}

bandwidth_vars_t reservation_getuResBandwidth() {
  return reservation_vars.bandwidth_vars;
}

void    reservation_setuResCommandID(uint8_t commandID){
  reservation_vars.commandID = commandID;
}

void    reservation_setuResBandwidth(uint8_t numOfLinks, uint8_t slotframeID){
  reservation_vars.bandwidth_vars.numOfLinks    = numOfLinks;
  reservation_vars.bandwidth_vars.slotframeID   = slotframeID;
}
//call res layer
void    reservation_notifyReceiveuResCommand(){
  
      uResCommandIEcontent_t* tempuResCommandIEcontent = processIE_getuResCommandIEcontent();
      switch(tempuResCommandIEcontent->uResCommandID)
      {
      case 0:break;
      case 1:break;
      case 2:break;
      case 3:break;
      case 4:break;
      default:
         // log the error
        break;
      }
}
//call by up layer
void reservation_linkRequest() {
  
  leds_debug_toggle();
  
  //this should added to openwsn
  uint8_t COMPONENT_RESERVATION = 0xff;
  OpenQueueEntry_t* reservationPkt;
  open_addr_t*      reservationNeighAddr;
  
  reservationNeighAddr = neighbors_reservationNeighbor();
  if(reservationNeighAddr!=NULL){
    // get a free packet buffer
    reservationPkt = openqueue_getFreePacketBuffer(COMPONENT_RESERVATION);
  
    if (reservationPkt==NULL) {
      openserial_printError(COMPONENT_RESERVATION,ERR_NO_FREE_PACKET_BUFFER,
                            (errorparameter_t)0,
                            (errorparameter_t)0);
      return;
    }
  }
  // declare ownership over that packet
  reservationPkt->creator = COMPONENT_RESERVATION;
  reservationPkt->owner   = COMPONENT_RESERVATION;
         
  memcpy(&(reservationPkt->l2_nextORpreviousHop),reservationNeighAddr,sizeof(open_addr_t));
  
  //set uRes command ID
  reservation_setuResCommandID(RESERCATIONLINKREQ);
  //set slotframeID and bandwidth
  reservation_setuResBandwidth(1,0);
  //set LinkTypeIE
  processIE_setSubuResLinkTypeIE();
  //set uResCommandIE
  processIE_setSubuResCommandIE();
  //set uResBandwidthIE
  processIE_setSubuResBandwidthIE();
  //set IE after set all required subIE
  processIE_setMLME_IE();
  //add an IE to adv's payload
  IEFiled_prependIE(reservationPkt);
  
  res_send(reservationPkt);
}

//event
void isr_reservation_button() {
  reservation_linkRequest();
}