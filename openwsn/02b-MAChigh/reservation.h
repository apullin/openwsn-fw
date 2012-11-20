#ifndef __RESERVATION_H
#define __RESERVATION_H

#include "openwsn.h"

//=========================== typedef =========================================
enum uResCommandID_num{
  RESERCATIONLINKREQ            = 0x00,
  RESERCATIONLINKRESPONSE       = 0x01,
  RESERVATIONREMOVELINKREQUEST  = 0x02,
  RESERVATIONSCHEDULERREQUEST   = 0x03,
  RESERVATIONSCHEDULERESPONSE   = 0x04,
};

typedef struct {
  uint8_t numOfLinks;
  uint8_t slotframeID;
} bandwidth_vars_t;
//=========================== variables =======================================

//=========================== prototypes ======================================
//admin
void             reservation_init();
//public
uint8_t          reservation_getuResCommandID();
bandwidth_vars_t reservation_getuResBandwidth();

void             reservation_setuResCommandID(uint8_t commandID);
void             reservation_setuResBandwidth(uint8_t numOfLinks, uint8_t slotframeID);
// call by res
void             reservation_notifyReceiveuResCommand();
// call by up layer
void             reservation_linkRequest();
// events
void             isr_reservation_button();
#endif