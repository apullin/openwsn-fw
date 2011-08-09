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
void activity_rie2();
void activity_ri4();
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
         activity_tie2();
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
   switch (state) {
      case S_TXDATADELAY:
         activity_ti4();
         break;
      case S_RXACKLISTEN:
         activity_ti8();
         break;
      case S_RXDATALISTEN:
         activity_ri4();
         break;
      case S_TXACKDELAY:
         activity_ri8();
         break;
      default:
         // poipoi
         break;
   }
}

// end of frame event
void tsch_endOfFrame() {
   switch (state) {
      case S_TXDATA:
         activity_ti5();
         break;
      case S_RXACK:
         activity_ti9();
         break;
      case S_RXDATA:
         activity_ri5();
         break;
      case S_TXACK:
         activity_ri9();
         break;
      default:
         // poipoi
         break;
   }
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
   switch (schedule_getSlotType(asn)) {
      case OFF:
         // I have nothing to do
         // abort
         endSlot();
         break;
      case TX:
         dataToSend = queue_getPacket(schedule_getNeighbor(ASN));
         if (dataToSend!=NULL) {
            // I have a packet to send
            // change state
            change_state(S_TXDATAOFFSET);
            // arm tt1
            tsch_timer_schedule(DURATION_tt1);
         }
         break;
      case RX:
         // I need to listen for packet
         // change state
         change_state(S_RXDATAOFFSET);
         // arm rt1
         tsch_timer_schedule(DURATION_rt1);
         break;
   }
}

inline void activity_ti2() {
      // change state
   change_state(S_TXDATAPREPARE);

   // calculate the frequency to transmit on
   frequency = calculateFrequency(asn, schedule_getChannelOffset(asn) );

   // configure the radio for that frequency
   radio_setFrequency(frequency);

   // copy the packet to send to the radio
   radio_loadPacket(dataToSend);

   // enable the radio in Tx mode. This does not send that packet.
   radio_txEnable();

   // arm tt2
   tsch_timer_schedule(DURATION_tt2);

   // change state
   change_state(S_TXDATAREADY);
}

inline void activity_tie1() {
   // log the error
   log_error(MAXTXDATAPREPAREOVERFLOW);

   // abort
   endSlot();
}

inline void activity_ti3() {
   // change state
   change_state(S_TXDATADELAY);

   // give the 'go' to transmit
   radio_txNow();

   // arm tt3
   tsch_timer_schedule(DURATION_tt3);
}

inline void activity_tie2() {
   // log the error
   log_error(WDRADIOTX);

   // abort
   endSlot();
}

inline void activity_ti4() {
   // change state
   change_state(S_TXDATA);

   // cancel the radio watchdog timer
   tsch_timer_cancel();

   // record the captured time
   capturedTime = getCapturedTime();

   // arm tt4
   tsch_timer_schedule(DURATION_tt4);
}

inline void activity_tie3() {
   // log the error
   log_error(WDDATADURATION);

   // abort
   endSlot();
}

inline void activity_ti5() {
   // update state
   state = RXACKOFFSET;

   // record the captured time
   capturedTime = getCapturedTime();

   // decides whether to listen for an ACK
   if (dataToSend->destination!=0xff) {
      listenForAck = TRUE;
   } else {
      listenForAck = FALSE;
   }

   if (listenForAck===TRUE) {
      // arm tt5
      tsch_timer_schedule(DURATION_tt5);
   } else {
      // indicate that the packet was sent successfully
      upperlater_txDone(dataToSend,SUCCESS);
      // reset local variable
      dataToSend = NULL;
      // abort
      endSlot();
   }
}

inline void activity_ti6() {
   // change state
   change_state(S_RXACKREADY);

   // calculate the frequency to transmit on
   frequency = calculateFrequency(asn, schedule_getChannelOffset(asn));

   // configure the radio for that frequency
   radio_setFrequency(frequency);

   // enable the radio in Rx mode. The radio is not actively listening yet.
   radio_rxEnable();

   // arm tt6
   tsch_timer_schedule(DURATION_tt6);
}

inline void activity_tie4() {
   // log the error
   log_error(MAXRXACKPREPAREOVERFLOW);

   // abort
   endSlot();
}

inline void activity_ti7() {
   // change state
   change_state(S_RXACKLISTEN);

   // start listening
   radio_rxNow();

   // arm tt7
   tsch_timer_schedule(DURATION_tt7);
}

inline void activity_tie5() {
   // transmit failed, decrement transmits left counter
   dataToSend->transmitsLeft--;

   // indicate tx fail if no more retries left
   if (dataToSend->transmitsLeft===0) {
      upperlayer_txDone(dataToSend,FAIL);
   }

   // reset local variable
   dataToSend = NULL;

   // abort
   endSlot();
}

inline void activity_ti8() {
   // change state
   change_state(S_TXACK);

   // record the captured time
   capturedTime = getCapturedTime();

   // cancel tt7
   tsch_timer_cancel();

   // arm tt8
   tsch_timer_schedule(DURATION_tt8);
}

inline void activity_tie6() {
   // abort
   endSlot();
}

inline void activity_ti9() {
   // update state
   state = TXPROC

   // cancel tt8
   tsch_timer_cancel();

   // record the captured time
   capturedTime = getCapturedTime();

   // retrieve the ACK frame
   get_getReceivedFrame(ackReceived);

   // check that ACK frame is valid
   if (isAckValid(ackReceived)) {
      validAck = TRUE;
   } else {
      validAck = FALSE;
   }

   // free the received frame so corresponding RAM memory can be recycled
   free(ackReceived);

   // if packet sent successfully, inform upper layer
   if (validAck===TRUE) {
      upperlayer_txDone(dataToSend,SUCCESS);
      dataToSend = NULL;
   }

   // official end of Tx slot
   endSlot();
}

//===================================== TX =====================================

inline void activity_ri2() {
   // change state
   change_state(S_RXDATAPREPARE);

   // calculate the frequency to transmit on
   frequency = calculateFrequency(asn, schedule_getChannelOffset(asn) );

   // configure the radio for that frequency
   radio_setFrequency(frequency);

   // enable the radio in Rx mode. The radio does not actively listen yet.
   radio_rxEnable();

   // arm rt2
   tsch_timer_schedule(DURATION_rt2);

   // change state
   change_state(S_RXDATAREADY);
}

inline void activity_rie1() {
   // log the error
   log_error(MAXRXDATAPREPAREOVERFLOW);

   // abort
   endSlot();
}

inline void activity_ri3() {
   // change state
   change_state(S_RXDATALISTEN);

   // give the 'go' to receive
   radio_rxNow();

   // arm rt3
   tsch_timer_schedule(DURATION_rt3);
}

inline void activity_rie2() {
   // abort
   endSlot();
}

inline void activity_ri4() {
   // change state
   change_state(S_RXDATA);

   // record the captured time
   capturedTime = getCapturedTime();

   // cancel rt3
   tsch_timer_cancel();

   // arm rt4
   tsch_timer_schedule(DURATION_rt4);
}

inline void activity_rie3() {
   // log the error
   log_error(WDDATADURATION);

   // abort
   endSlot();
}

inline void activity_ri5() {
   // update state
   state = TXACK OFFSET

   // cancel rt4
   tsch_timer_cancel();

   // record the captured time
   capturedTime = getCapturedTime();

   // retrieve the ACK frame
   get_getReceivedFrame(dataReceived);

   // if data frame invalid, stop
   if (isDataValid(dataReceived)==FALSE) {
      // free the buffer
      free(dataReceived);
      
      // clear local variable
      dataReceived = NULL;

      // abort
      endSlot();
      return;
   }

   // check if ack requested
   if (ackRequested(dataReceived)==TRUE) {
      // arm rt5
      tsch_timer_schedule(DURATION_rt5);
   } else {
      // indicate reception to upper layer
      upperlayer_receive(dataReceived);
      // reset local variable
      dataReceived = NULL;
      // abort
      endSlot();
   }
}

inline void activity_ri6() {
   // change state
   change_state(S_TXACKPREPARE);

   // get a buffer to put the ack in
   ackToSend = getFreeBuffer();
   if (ackToSend==NULL) {
      // indicate we received a packet ()
      upperlayer_receive(dataReceived);
      // free local variable
      dataReceived = NULL;
      // abort
      endSlot();
   }

   // create the ACK
   createAck(ackToSend);

   // calculate the frequency to transmit on
   frequency = calculateFrequency(asn, schedule_getChannelOffset(asn) );

   // configure the radio for that frequency
   radio_setFrequency(frequency);

   // copy the packet to send to the radio
   radio_loadPacket(ackToSend);

   // enable the radio in Tx mode. This does not send that packet.
   radio_txEnable();

   // arm rt6
   tsch_timer_schedule(DURATION_rt6);

   // change state
   change_state(S_TXACKREADY);
}

inline void activity_rie4() {
   // log the error
   log_error(MAXTXACKPREPAREOVERFLOW);

   // abort
   endSlot();
}

inline void activity_ri7() {
   // change state
   change_state(S_TXACKDELAY);

   // give the 'go' to transmit
   radio_txNow();

   // arm rt7
   tsch_timer_schedule(DURATION_rt7);
}

inline void activity_rie5() {
   // log the error
   log_error(WDRADIOTX);

   // abort
   endSlot();
}

inline void activity_ri8() {
   // change state
   change_state(S_TXACK);

   // cancel rt7
   tsch_timer_cancel();

   // record the captured time
   capturedTime = getCapturedTime();

   // arm rt8
   tsch_timer_schedule(DURATION_rt8);
}

inline void activity_rie6() {
   // log the error
   log_error(WDACKDURATION);

   // abort
   endSlot();
}

inline void activity_ri9() {
   // update state
   state = RXPROC

   // cancel rt8
   tsch_timer_cancel();

   // record the captured time
   capturedTime = getCapturedTime();

   // inform upper layer of reception
   upperlayer_receive(dataReceive);

   // clear local variable
   dataReceive== NULL;

   // official end of Rx slot
   endSlot();
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
   // turn off the radio
   radio_rfOff();

   // reset capturedTime
   capturedTime = 0;

   // clean up dataToSend
   if (dataToSend!=NULL) {
      // if everything went well, dataToSend was set to NULL in ti9
      // transmit failed, decrement transmits left counter
      dataToSend->transmitsLeft--;
      // indicate tx fail if counnter 
      if (dataToSend->transmitsLeft===0) {
         upperlayer_txDone(dataToSend,FAIL);
      }
      // reset local variable
      dataToSend = NULL;
   }

   // clean up dataReceived
   if (dataReceived!=NULL) {
      // assume something went wrong. If everything went well, dataReceived would have been set to NULL in ri9.
      // indicate  "received packet" to upper layer; we don't want to loose packets
      upperlayer_receive(dataReceived);
      // reset local variable
      dataReceived = NULL;
   }

   // clean up ackToSend
   if (ackToSend!=NULL) {
      // free ackToSend
      free(ackToSend);
      // reset local variable
      ackToSend = NULL;
   }

   // clean up ackReceived
   if (ackReceived!=NULL) {
      // free ackReceived
      free(ackReceived);
      // reset local variable
      ackReceived = NULL;
   }
}

void radio_packet_received(OpenQueueEntry_t* msg) {
}

bool mac_debugPrint() {
   return FALSE;
}