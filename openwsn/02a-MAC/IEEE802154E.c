/*
 * IEEE802.15.4e TSCH
 *
 * Authors:
 * Branko Kerkez   <bkerkeze@berkeley.edu>, March 2011
 * Fabien Chraim   <chraim@eecs.berkeley.edu>, June 2011
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
 */

#include "openwsn.h"
#include "IEEE802154E.h"
#include "IEEE802154.h"
#include "radio.h"
#include "packetfunctions.h"
#include "idmanager.h"
#include "openserial.h"
#include "openqueue.h"
#include "schedule.h"
#include "tsch_timer.h"
#include "packetfunctions.h"
#include "neighbors.h"
#include "nores.h"

//===================================== variables ==============================

asn_t              asn;
uint8_t            state;
uint8_t            dsn;
OpenQueueEntry_t*  dataToSend;
OpenQueueEntry_t*  dataReceived;
OpenQueueEntry_t*  ackToSend;
OpenQueueEntry_t*  ackReceived;
timestamp_t        capturedTime;
bool               isSync;

//===================================== prototypes =============================

#include "IEEE802154_common.c"
// TX
void activity_ti1ORri1();
void activity_ti2();
void activity_tie1();
void activity_ti3();
void activity_tie2();
void activity_ti4();
void activity_tie3();
void activity_ti5();
void activity_ti6();
void activity_tie4();
void activity_ti7();
void activity_tie5();
void activity_ti8();
void activity_tie6();
void activity_ti9();
// RX
void activity_ri2();
void activity_rie1();
void activity_ri3();
void activity_ri4();
void activity_rie2();
void activity_rie3();
void activity_ri5();
void activity_ri6();
void activity_rie4();
void activity_ri7();
void activity_rie5();
void activity_ri8();
void activity_rie6();
void activity_ri9();
// helper
void change_state(uint8_t newstate);
void endSlot();
bool mac_debugPrint();

//===================================== public from upper layer ===============

void mac_init() {   
   // initialize debug pins
   DEBUG_PIN_FRAME_INIT();
   DEBUG_PIN_SLOT_INIT();
   DEBUG_PIN_FSM_INIT();
   
   // initialize variables
   asn                       = 0;
   state                     = SLEEP;
   dsn                       = 0;
   dataToSend                = NULL;
   dataReceived              = NULL;
   ackToSend                 = NULL;
   ackReceived               = NULL;
   capturedTime.valid        = TRUE;
   capturedTime.timestamp    = 0;
   isSync                    = FALSE;
   
   // initialize (and start) TSCH timer
   tsch_timer_init();
}

// a packet sent from the upper layer is simply stored into the OpenQueue buffer.
error_t mac_send(OpenQueueEntry_t* msg) {
   msg->owner = COMPONENT_MAC;
   if (packetfunctions_isBroadcastMulticast(&(msg->l2_nextORpreviousHop))==TRUE) {
      msg->l2_retriesLeft = 1;
   } else {
      msg->l2_retriesLeft = TXRETRIES;
   }
   msg->l1_txPower = TX_POWER;
   //IEEE802.15.4 header
   prependIEEE802154header(msg,
                           msg->l2_frameType,
                           IEEE154_SEC_NO_SECURITY,
                           dsn++,
                           &(msg->l2_nextORpreviousHop)
                           );
   // space for 2-byte CRC
   packetfunctions_reserveFooterSize(msg,2);
   return E_SUCCESS;
}

//===================================== public from lower layer ================

void mac_sendDone(OpenQueueEntry_t* pkt, error_t error) {
}

//===================================== events =================================

//new slot event
void tsch_newSlot() {
   activity_ti1ORri1();
}

//timer fires event
void tsch_timerFires() {
   switch (state) {
      case S_TXDATAOFFSET:
         activity_ti2();
         break;
      case S_TXDATAPREPARE:
         activity_tie1();
         break;
      case S_TXDATAREADY:
         activity_ti3();
         break;
      case S_TXDATADELAY:
         activity_ti4();
         break;
      case S_TXDATA:
         activity_tie3();
         break;
      case S_RXACKOFFSET:
         activity_ti6();
         break;
      case S_RXACKPREPARE:
         activity_tie4();
         break;
      case S_RXACKREADY:
         activity_ti7();
         break;
      case S_RXACKLISTEN:
         activity_tie5();
         break;
      case S_RXACK:
         activity_tie6();
         break;
      case S_RXDATAOFFSET:
         activity_ri2();
         break;
      case S_RXDATAPREPARE:
         activity_rie1();
         break;
      case S_RXDATAREADY:
         activity_ri3();
         break;
      case S_RXDATALISTEN:
         activity_rie2();
         break;
      case S_RXDATA:
         activity_rie3();
         break;
      case S_TXACKOFFSET:
         activity_ri6();
         break;
      case S_TXACKPREPARE:
         activity_rie4();
         break;
      case S_TXACKREADY:
         activity_ri7();
         break;
      case S_TXACKDELAY:
         activity_rie5();
         break;
      case S_TXACK:
         activity_rie6();
         break;
      case S_SLEEP:
      case S_SYNCHRONIZING:
      case S_TXPROC:
      case S_RXPROC:
      default:
         // poipoi
         break;
   }
}

// start of frame event
void tsch_startOfFrame() {
   
}

// end of frame event
void tsch_endOfFrame() {
   
}

//===================================== TX =====================================

inline void activity_ti1ORri1() {
   // wiggle debug pins
   DEBUG_PIN_SLOT_TOGGLE();
   if (asn%SCHEDULELENGTH==0) {
      DEBUG_PIN_FRAME_TOGGLE();
   }
   
   // increment ASN
   asn++;
   
   // if the previous slot took too long, we will not be in the right state
   if (state!=S_SLEEP) {
      // log the error
      openserial_printError(COMPONENT_MAC,
                            ERR_WRONG_STATE_IN_STARTSLOT,
                            state,
                            asn%SCHEDULELENGTH);
      
      // abort
      endSlot();
      return;
   }
   
   // check the schedule to see what type of slot this is
   switch (schedule_getType(asn)) {
      case CELLTYPE_OFF:
         // I have nothing to do
         // abort
         endSlot();
         break;
      case CELLTYPE_TX:
         dataToSend = queue_getPacket(schedule_getNeighbor(ASN));
         if (dataToSend!=NULL) {
            // I have a packet to send
            // change state
            change_state(S_TXDATAOFFSET);
            // arm tt1
            tsch_timer_schedule(DURATION_tt1);
         }
         break;
      case CELLTYPE_RX:
         // I need to listen for packet
         // change state
         change_state(S_RXDATAOFFSET);
         // arm rt1
         tsch_timer_schedule(DURATION_rt1);
         break;
   }
}

inline void activity_ti1() {
   // TODO
}

inline void activity_ti2() {
   // change state
   change_state(S_SLEEP);

   // TODO
}

inline void activity_tie1() {
   // TODO
}

inline void activity_ti3() {
   // TODO
}

inline void activity_tie2() {
   // TODO
}

inline void activity_ti4() {
   // TODO
}

inline void activity_tie3() {
   // TODO
}

inline void activity_ti5() {
   // TODO
}

inline void activity_ti6() {
   // TODO
}

inline void activity_tie4() {
   // TODO
}

inline void activity_ti7() {
   // TODO
}

inline void activity_tie5() {
   // TODO
}

inline void activity_ti8() {
   // TODO
}

inline void activity_tie6() {
   // TODO
}

inline void activity_ti9() {
   // TODO
}

//===================================== TX =====================================

inline void activity_ri2() {
   // TODO
}

inline void activity_rie1() {
   // TODO
}

inline void activity_ri3() {
   // TODO
}

inline void activity_ri4() {
   // TODO
}

inline void activity_rie2() {
   // TODO
}

inline void activity_rie3() {
   // TODO
}

inline void activity_ri5() {
   // TODO
}

inline void activity_ri6() {
   // TODO
}

inline void activity_rie4() {
   // TODO
}

inline void activity_ri7() {
   // TODO
}

inline void activity_rie5() {
   // TODO
}

inline void activity_ri8() {
   // TODO
}

inline void activity_rie6() {
   // TODO
}

inline void activity_ri9() {
   // TODO
}

void change_state(uint8_t newstate) {
   state = newstate;
   switch (state) {
      case S_SLEEP:
      case S_RXDATAOFFSET:
         DEBUG_PIN_FSM_CLR();
         break;
      case S_SYNCHRONIZING:
      case S_TXDATAOFFSET:
         DEBUG_PIN_FSM_SET();
         break;
      case S_TXDATAPREPARE:
      case S_TXDATAREADY:
      case S_TXDATADELAY:
      case S_TXDATA:
      case S_RXACKOFFSET:
      case S_RXACKPREPARE:
      case S_RXACKREADY:
      case S_RXACKLISTEN:
      case S_RXACK:
      case S_TXPROC:
      case S_RXDATAPREPARE:
      case S_RXDATAREADY:
      case S_RXDATALISTEN:
      case S_RXDATA:
      case S_TXACKOFFSET:
      case S_TXACKPREPARE:
      case S_TXACKREADY:
      case S_TXACKDELAY:
      case S_TXACK:
      case S_RXPROC:
         DEBUG_PIN_FSM_TOGGLE();
         break;
   }
}

void endSlot() {
   // TODO
}

void radio_packet_received(OpenQueueEntry_t* msg) {
}

void notifyReceive() {
}

bool mac_debugPrint() {
   return FALSE;
}