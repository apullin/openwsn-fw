/**
\brief IEEE802.15.4e TSCH

\author Branko Kerkez <bkerkez@berkeley.edu>, March 2011
\author Fabien Chraim <chraim@eecs.berkeley.edu>, June 2011
\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
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
#include "ieee154etimer.h"
#include "packetfunctions.h"
#include "neighbors.h"
#include "res.h"

//=========================== variables =======================================

typedef struct {
   asn_t              asn;                // current absolute slot number
   uint8_t            state;              // state of the FSM
   uint8_t            dsn;                // data sequence number
   uint16_t           capturedTime;       // last captures time
   bool               isSync;             // TRUE iff mote synchronized to network
   OpenQueueEntry_t*  dataToSend;         // pointer to the data to send
   OpenQueueEntry_t*  dataReceived;       // pointer to the data received
   OpenQueueEntry_t*  ackToSend;          // pointer to the ack to send
   OpenQueueEntry_t*  ackReceived;        // pointer to the ack received
} ieee154e_vars_t;

ieee154e_vars_t ieee154e_vars;

//=========================== prototypes ======================================

// SYNCHRONIZING
void    activity_synchronize_newSlot();
void    activity_synchronize_startOfFrame(uint16_t capturedTime);
void    activity_synchronize_endOfFrame(uint16_t capturedTime);
// TX
void    activity_ti1ORri1();
void    activity_ti2();
void    activity_tie1();
void    activity_ti3();
void    activity_tie2();
void    activity_ti4(uint16_t capturedTime);
void    activity_tie3();
void    activity_ti5(uint16_t capturedTime);
void    activity_ti6();
void    activity_tie4();
void    activity_ti7();
void    activity_tie5();
void    activity_ti8(uint16_t capturedTime);
void    activity_tie6();
void    activity_ti9(uint16_t capturedTime);
// RX
void    activity_ri2();
void    activity_rie1();
void    activity_ri3();
void    activity_rie2();
void    activity_ri4(uint16_t capturedTime);
void    activity_rie3();
void    activity_ri5(uint16_t capturedTime);
void    activity_ri6();
void    activity_rie4();
void    activity_ri7();
void    activity_rie5();
void    activity_ri8(uint16_t capturedTime);
void    activity_rie6();
void    activity_ri9(uint16_t capturedTime);
// helper
uint8_t calculateFrequency(asn_t asn, uint8_t channelOffset);
bool    isAckValid(OpenQueueEntry_t* ackFrame);
bool    isDataValid(OpenQueueEntry_t* dataFrame);
bool    ackRequested(OpenQueueEntry_t* frame);
void    createAck(OpenQueueEntry_t* frame);
void    change_state(uint8_t newstate);
void    endSlot();
bool    mac_debugPrint();

//=========================== public ==========================================

//======= from upper layer

/**
\brief This function initializes this module.

Call this function once before any other function in this module, possibly
during boot-up.
*/
void mac_init() {   
   // initialize debug pins
   DEBUG_PIN_FRAME_INIT();
   DEBUG_PIN_SLOT_INIT();
   DEBUG_PIN_FSM_INIT();
   
   // initialize variables
   ieee154e_vars.asn                       = 0;
   ieee154e_vars.state                     = S_SLEEP;
   ieee154e_vars.dsn                       = 0;
   ieee154e_vars.dataToSend                = NULL;
   ieee154e_vars.dataReceived              = NULL;
   ieee154e_vars.ackToSend                 = NULL;
   ieee154e_vars.ackReceived               = NULL;
   ieee154e_vars.capturedTime              = 0;
   if (idmanager_getIsDAGroot()==TRUE) {
      ieee154e_vars.isSync                 = TRUE;
   } else {
      ieee154e_vars.isSync                 = FALSE;
   }
   
   // initialize (and start) IEEE802.15.4e timer
   ieee154etimer_init();
}

/**
\brief Function called by the upper layer when it wants to send a packet

This function adds a IEEE802.15.4 header to the packet and leaves it the 
OpenQueue buffer, waiting to be transmitted.

\param [in] msg The packet to the transmitted

\returns E_SUCCESS iff successful.
*/
error_t mac_send(OpenQueueEntry_t* msg) {
   msg->owner = COMPONENT_IEEE802154E;
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
                           ieee154e_vars.dsn++,
                           &(msg->l2_nextORpreviousHop)
                           );
   // space for 2-byte CRC
   packetfunctions_reserveFooterSize(msg,2);
   return E_SUCCESS;
}

//======= events

/**
\brief Indicates a new slot has just started.

This function executes in ISR mode, when the new slot timer fires.
*/
void isr_ieee154e_newSlot() {
   if (ieee154e_vars.isSync==FALSE) {
      activity_synchronize_newSlot();
   } else {
      activity_ti1ORri1();
   }
}

/**
\brief Indicates the FSM timer has fired.

This function executes in ISR mode, when the FSM timer fires.
*/
void isr_ieee154e_timer() {
   switch (ieee154e_vars.state) {
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
      default:
         // log the error
         openserial_printError(COMPONENT_IEEE802154E,
                               ERR_WRONG_STATE_IN_TIMERFIRES,
                               ieee154e_vars.state,
                               ieee154e_vars.asn%SCHEDULELENGTH);
         // abort
         endSlot();
         break;
   }
}

/**
\brief Indicates the radio just received the first byte of a packet.

This function executes in ISR mode.
*/
void ieee154e_startOfFrame(uint16_t capturedTime) {
   if (ieee154e_vars.isSync==FALSE) {
      activity_synchronize_startOfFrame(capturedTime);
   } else {
      switch (ieee154e_vars.state) {
         case S_TXDATADELAY:
            activity_ti4(capturedTime);
            break;
         case S_RXACKLISTEN:
            activity_ti8(capturedTime);
            break;
         case S_RXDATALISTEN:
            activity_ri4(capturedTime);
            break;
         case S_TXACKDELAY:
            activity_ri8(capturedTime);
            break;
         default:
            // log the error
            openserial_printError(COMPONENT_IEEE802154E,
                                  ERR_WRONG_STATE_IN_NEWSLOT,
                                  ieee154e_vars.state,
                                  ieee154e_vars.asn%SCHEDULELENGTH);
            // abort
            endSlot();
            break;
      }
   }
}

/**
\brief Indicates the radio just received the last byte of a packet.

This function executes in ISR mode.
*/
void ieee154e_endOfFrame(uint16_t capturedTime) {
   if (ieee154e_vars.isSync==FALSE) {
      activity_synchronize_endOfFrame(capturedTime);
   } else {
      switch (ieee154e_vars.state) {
         case S_TXDATA:
            activity_ti5(capturedTime);
            break;
         case S_RXACK:
            activity_ti9(capturedTime);
            break;
         case S_RXDATA:
            activity_ri5(capturedTime);
            break;
         case S_TXACK:
            activity_ri9(capturedTime);
            break;
         default:
            // log the error
            openserial_printError(COMPONENT_IEEE802154E,
                                  ERR_WRONG_STATE_IN_ENDOFFRAME,
                                  ieee154e_vars.state,
                                  ieee154e_vars.asn%SCHEDULELENGTH);
            // abort
            endSlot();
            break;
      }
   }
}

//======= misc

bool mac_debugPrint() {
   return FALSE;
}

//=========================== private =========================================

//============ SYNCHRONIZING

inline void activity_synchronize_newSlot() {
   // if this is the first time I call this function while not synchronized,
   // switch on the radio in Rx mode
   if (ieee154e_vars.state!=S_SYNCHRONIZING) {
      // change state
      change_state(S_SYNCHRONIZING);
      
      // turn off the radio (in case it wasn't yet)
      radio_rfOff();
      
      // configure the radio to listen to the default synchronizing channel
      radio_setFrequency(SYNCHRONIZING_CHANNEL);
      
      // switch on the radio in Rx mode.
      radio_rxEnable();
      radio_rxNow();
   }
}

inline void activity_synchronize_startOfFrame(uint16_t capturedTime) {
   // get the captured time 
   ieee154e_vars.capturedTime = capturedTime;
}

inline void activity_synchronize_endOfFrame(uint16_t capturedTime) {
   ieee802154_header_iht ieee802514_header;
   
   // get a buffer to put the (received) frame in
   ieee154e_vars.dataReceived = openqueue_getFreePacketBuffer();
   if (ieee154e_vars.dataReceived==NULL) {
      // log the error
      openserial_printError(COMPONENT_IEEE802154E,
                            ERR_NO_FREE_PACKET_BUFFER,
                            0,
                            0);
      // abort
      endSlot();
      return;
   }
   
   // retrieve the packet from the radio's Rx buffer
   radio_getReceivedFrame(ieee154e_vars.dataReceived);
   
   // parse the packet
   retrieveIEEE802154header(ieee154e_vars.dataReceived,&ieee802514_header);
   
   // if it's a valid ADV, synchronize
   if (ieee802514_header.valid==TRUE                                                                    &&
       ieee802514_header.frameType==IEEE154_TYPE_BEACON                                                 &&
       packetfunctions_sameAddress(&ieee802514_header.panid,idmanager_getMyID(ADDR_PANID))              &&
       ieee154e_vars.dataReceived->length==ieee802514_header.headerLength+sizeof(IEEE802154E_ADV_t)+2) {
      
      // toss the IEEE802.15.4 header
      packetfunctions_tossHeader(ieee154e_vars.dataReceived,ieee802514_header.headerLength);
      
      // synchronize the slots to the sender's
      // TODO
         
      // record the ASN
      ieee154e_vars.asn = ((IEEE802154E_ADV_t*)(ieee154e_vars.dataReceived->payload))->asn;
      
      // declare synchronized
      //poipoiisSync = TRUE;
      
      // turn radio off
      radio_rfOff();
      
      // change state
      change_state(S_SLEEP);
   }
   
   // free the received data buffer so corresponding RAM memory can be recycled
   openqueue_freePacketBuffer(ieee154e_vars.dataReceived);
   
}

//============ TX

inline void activity_ti1ORri1() {
   uint8_t cellType;
   open_addr_t neighbor;
   
   //stop outputting serial data
   openserial_stop();
   
   // increment ASN (do this first so debug pins are in sync)
   ieee154e_vars.asn++;
   
   // wiggle debug pins
   DEBUG_PIN_SLOT_TOGGLE();
   if (ieee154e_vars.asn%SCHEDULELENGTH==0) {
      DEBUG_PIN_FRAME_TOGGLE();
   }

   // if the previous slot took too long, we will not be in the right state
   if (ieee154e_vars.state!=S_SLEEP) {
      // log the error
      openserial_printError(COMPONENT_IEEE802154E,
                            ERR_WRONG_STATE_IN_STARTSLOT,
                            ieee154e_vars.state,
                            ieee154e_vars.asn%SCHEDULELENGTH);
      // abort
      endSlot();
      return;
   }

   // check the schedule to see what type of slot this is
   cellType = schedule_getType(ieee154e_vars.asn);
   switch (cellType) {
      case CELLTYPE_OFF:
         // I have nothing to do
         // abort
         endSlot();
         //start outputing serial
         openserial_startOutput();
         break;
      case CELLTYPE_ADV:
         ieee154e_vars.dataToSend = openqueue_getAdvPacket();
         if (ieee154e_vars.dataToSend==NULL) {
            // I will be listening for an ADV
            // change state
            change_state(S_RXDATAOFFSET);
            // arm rt1
            ieee154etimer_schedule(DURATION_rt1);
         } else {
            // I will be sending an ADV
            // change state
            change_state(S_TXDATAOFFSET);
            // arm tt1
            ieee154etimer_schedule(DURATION_tt1);
         }
         break;
      case CELLTYPE_TX:
         schedule_getNeighbor(ieee154e_vars.asn,&neighbor);
         ieee154e_vars.dataToSend = openqueue_getDataPacket(&neighbor);
         if (ieee154e_vars.dataToSend!=NULL) {
            // I have a packet to send
            // change state
            change_state(S_TXDATAOFFSET);
            // arm tt1
            ieee154etimer_schedule(DURATION_tt1);
         } else {
            // abort
            endSlot();
         }
         break;
      case CELLTYPE_RX:
         // I need to listen for packet
         // change state
         change_state(S_RXDATAOFFSET);
         // arm rt1
         ieee154etimer_schedule(DURATION_rt1);
         break;
      default:
         // log the error
         openserial_printError(COMPONENT_IEEE802154E,
                               ERR_WRONG_CELLTYPE,
                               cellType,
                               ieee154e_vars.asn%SCHEDULELENGTH);
         // abort
         endSlot();
         break;
   }
}

inline void activity_ti2() {
   uint8_t frequency;
   
   // change state
   change_state(S_TXDATAPREPARE);

   // calculate the frequency to transmit on
   frequency = calculateFrequency(ieee154e_vars.asn, schedule_getChannelOffset(ieee154e_vars.asn) );

   // configure the radio for that frequency
   radio_setFrequency(frequency);

   // copy the packet to send to the radio
   radio_loadPacket(ieee154e_vars.dataToSend);

   // enable the radio in Tx mode. This does not send the packet.
   radio_txEnable();

   // arm tt2
   ieee154etimer_schedule(DURATION_tt2);

   // change state
   change_state(S_TXDATAREADY);
}

inline void activity_tie1() {
   // log the error
   openserial_printError(COMPONENT_IEEE802154E,
                         ERR_MAXTXDATAPREPARE_OVERFLOW,
                         ieee154e_vars.state,
                         ieee154e_vars.asn%SCHEDULELENGTH);

   // abort
   endSlot();
}

inline void activity_ti3() {
   // change state
   change_state(S_TXDATADELAY);

   // give the 'go' to transmit
   radio_txNow();
   
   // arm tt3
   ieee154etimer_schedule(DURATION_tt3);
}

inline void activity_tie2() {
   // log the error
   openserial_printError(COMPONENT_IEEE802154E,
                         ERR_WDRADIO_OVERFLOW,
                         ieee154e_vars.state,
                         ieee154e_vars.asn%SCHEDULELENGTH);

   // abort
   endSlot();
}

inline void activity_ti4(uint16_t capturedTime) {
   // change state
   change_state(S_TXDATA);

   // cancel the radio watchdog timer
   ieee154etimer_cancel();

   // record the captured time
   ieee154e_vars.capturedTime = capturedTime;
   
   // arm tt4
   ieee154etimer_schedule(DURATION_tt4);
}

inline void activity_tie3() {
   // log the error
   openserial_printError(COMPONENT_IEEE802154E,
                         ERR_WDDATADURATION_OVERFLOWS,
                         ieee154e_vars.state,
                         ieee154e_vars.asn%SCHEDULELENGTH);

   // abort
   endSlot();
}

inline void activity_ti5(uint16_t capturedTime) {
   bool listenForAck;
   
   // change state
   change_state(S_RXACKOFFSET);
   
   // turn off the radio
   radio_rfOff();

   // record the captured time
   ieee154e_vars.capturedTime = capturedTime;

   // decides whether to listen for an ACK
   if (packetfunctions_isBroadcastMulticast(&ieee154e_vars.dataToSend->l2_nextORpreviousHop)==TRUE) {
      listenForAck = FALSE;
   } else {
      listenForAck = TRUE;
   }

   if (listenForAck==TRUE) {
      // arm tt5
      ieee154etimer_schedule(DURATION_tt5);
   } else {
      // indicate that the packet was sent successfully
      res_sendDone(ieee154e_vars.dataToSend,E_SUCCESS);
      // reset local variable
      ieee154e_vars.dataToSend = NULL;
      // abort
      endSlot();
   }
}

inline void activity_ti6() {
   uint8_t frequency;
   
   // change state
   change_state(S_RXACKPREPARE);

   // calculate the frequency to transmit on
   frequency = calculateFrequency(ieee154e_vars.asn, schedule_getChannelOffset(ieee154e_vars.asn));

   // configure the radio for that frequency
   radio_setFrequency(frequency);

   // enable the radio in Rx mode. The radio is not actively listening yet.
   radio_rxEnable();

   // arm tt6
   ieee154etimer_schedule(DURATION_tt6);
   
   // change state
   change_state(S_RXACKREADY);
}

inline void activity_tie4() {
   // log the error
   openserial_printError(COMPONENT_IEEE802154E,
                         ERR_MAXRXACKPREPARE_OVERFLOWS,
                         ieee154e_vars.state,
                         ieee154e_vars.asn%SCHEDULELENGTH);

   // abort
   endSlot();
}

inline void activity_ti7() {
   // change state
   change_state(S_RXACKLISTEN);

   // start listening
   radio_rxNow();

   // arm tt7
   ieee154etimer_schedule(DURATION_tt7);
}

inline void activity_tie5() {
   // transmit failed, decrement transmits left counter
   ieee154e_vars.dataToSend->l2_retriesLeft--;

   // indicate tx fail if no more retries left
   if (ieee154e_vars.dataToSend->l2_retriesLeft==0) {
      res_sendDone(ieee154e_vars.dataToSend,E_FAIL);
   }

   // reset local variable
   ieee154e_vars.dataToSend = NULL;

   // abort
   endSlot();
}

inline void activity_ti8(uint16_t capturedTime) {
   // change state
   change_state(S_TXACK);

   // record the captured time
   ieee154e_vars.capturedTime = capturedTime;

   // cancel tt7
   ieee154etimer_cancel();

   // arm tt8
   ieee154etimer_schedule(DURATION_tt8);
}

inline void activity_tie6() {
   // abort
   endSlot();
}

inline void activity_ti9(uint16_t capturedTime) {
   bool validAck;
   
   // change state
   change_state(S_TXPROC);

   // cancel tt8
   ieee154etimer_cancel();

   // record the captured time
   ieee154e_vars.capturedTime = capturedTime;
   
   // get a buffer to put the (received) ACK in
   ieee154e_vars.ackReceived = openqueue_getFreePacketBuffer();
   if (ieee154e_vars.ackReceived==NULL) {
      // log the error
      openserial_printError(COMPONENT_IEEE802154E,
                            ERR_NO_FREE_PACKET_BUFFER,
                            0,
                            0);
      // abort
      endSlot();
      return;
   }
   
   // retrieve the ACK frame
   radio_getReceivedFrame(ieee154e_vars.ackReceived);

   // check that ACK frame is valid
   if (isAckValid(ieee154e_vars.ackReceived)) {
      validAck = TRUE;
   } else {
      validAck = FALSE;
   }

   // free the received ack so corresponding RAM memory can be recycled
   openqueue_freePacketBuffer(ieee154e_vars.ackReceived);

   // if packet sent successfully, inform upper layer
   if (validAck==TRUE) {
      res_sendDone(ieee154e_vars.dataToSend,E_SUCCESS);
      ieee154e_vars.dataToSend = NULL;
   }

   // official end of Tx slot
   endSlot();
}

//============ RX

inline void activity_ri2() {
   uint8_t frequency;
   
   // change state
   change_state(S_RXDATAPREPARE);

   // calculate the frequency to transmit on
   frequency = calculateFrequency(ieee154e_vars.asn, schedule_getChannelOffset(ieee154e_vars.asn) );

   // configure the radio for that frequency
   radio_setFrequency(frequency);

   // enable the radio in Rx mode. The radio does not actively listen yet.
   radio_rxEnable();

   // arm rt2
   ieee154etimer_schedule(DURATION_rt2);

   // change state
   change_state(S_RXDATAREADY);
}

inline void activity_rie1() {
   // log the error
   openserial_printError(COMPONENT_IEEE802154E,
                         ERR_MAXRXDATAPREPARE_OVERFLOWS,
                         ieee154e_vars.state,
                         ieee154e_vars.asn%SCHEDULELENGTH);

   // abort
   endSlot();
}

inline void activity_ri3() {
   // change state
   change_state(S_RXDATALISTEN);

   // give the 'go' to receive
   radio_rxNow();

   // arm rt3
   ieee154etimer_schedule(DURATION_rt3);
}

inline void activity_rie2() {
   // abort
   endSlot();
}

inline void activity_ri4(uint16_t capturedTime) {
   // change state
   change_state(S_RXDATA);

   // record the captured time
   ieee154e_vars.capturedTime = capturedTime;

   // cancel rt3
   ieee154etimer_cancel();

   // arm rt4
   ieee154etimer_schedule(DURATION_rt4);
}

inline void activity_rie3() {
   // log the error
   openserial_printError(COMPONENT_IEEE802154E,
                         ERR_WDDATADURATION_OVERFLOWS,
                         ieee154e_vars.state,
                         ieee154e_vars.asn%SCHEDULELENGTH);
   
   // abort
   endSlot();
}

inline void activity_ri5(uint16_t capturedTime) {
   // change state
   change_state(S_TXACKOFFSET);

   // cancel rt4
   ieee154etimer_cancel();

   // record the captured time
   ieee154e_vars.capturedTime = capturedTime;

   // get a buffer to put the (received) data in
   ieee154e_vars.dataReceived = openqueue_getFreePacketBuffer();
   if (ieee154e_vars.dataReceived==NULL) {
      // log the error
      openserial_printError(COMPONENT_IEEE802154E,
                            ERR_NO_FREE_PACKET_BUFFER,
                            0,
                            0);
      // abort
      endSlot();
      return;
   }
   
   // retrieve the data frame
   radio_getReceivedFrame(ieee154e_vars.dataReceived);

   // if data frame invalid, stop
   if (isDataValid(ieee154e_vars.dataReceived)==FALSE) {
      // free the received data so corresponding RAM memory can be recycled
      openqueue_freePacketBuffer(ieee154e_vars.dataReceived);
      
      // clear local variable
      ieee154e_vars.dataReceived = NULL;

      // abort
      endSlot();
      return;
   }

   // check if ack requested
   if (ackRequested(ieee154e_vars.dataReceived)==TRUE) {
      // arm rt5
      ieee154etimer_schedule(DURATION_rt5);
   } else {
      // indicate reception to upper layer
      res_receive(ieee154e_vars.dataReceived);
      // reset local variable
      ieee154e_vars.dataReceived = NULL;
      // abort
      endSlot();
   }
}

inline void activity_ri6() {
   uint8_t frequency;
   
   // change state
   change_state(S_TXACKPREPARE);

   // get a buffer to put the ack to send in
   ieee154e_vars.ackToSend = openqueue_getFreePacketBuffer();
   if (ieee154e_vars.ackToSend==NULL) {
      // log the error
      openserial_printError(COMPONENT_IEEE802154E,
                            ERR_NO_FREE_PACKET_BUFFER,
                            0,
                            0);
      // indicate we received a packet (we don't want to loose any)
      res_receive(ieee154e_vars.dataReceived);
      // free local variable
      ieee154e_vars.dataReceived = NULL;
      // abort
      endSlot();
      return;
   }

   // create the ACK
   createAck(ieee154e_vars.ackToSend);

   // calculate the frequency to transmit on
   frequency = calculateFrequency(ieee154e_vars.asn, schedule_getChannelOffset(ieee154e_vars.asn) );

   // configure the radio for that frequency
   radio_setFrequency(frequency);

   // copy the packet to send to the radio
   radio_loadPacket(ieee154e_vars.ackToSend);

   // enable the radio in Tx mode. This does not send that packet.
   radio_txEnable();

   // arm rt6
   ieee154etimer_schedule(DURATION_rt6);

   // change state
   change_state(S_TXACKREADY);
}

inline void activity_rie4() {
   // log the error
   openserial_printError(COMPONENT_IEEE802154E,
                         ERR_MAXTXACKPREPARE_OVERFLOWS,
                         ieee154e_vars.state,
                         ieee154e_vars.asn%SCHEDULELENGTH);

   // abort
   endSlot();
}

inline void activity_ri7() {
   // change state
   change_state(S_TXACKDELAY);

   // give the 'go' to transmit
   radio_txNow();

   // arm rt7
   ieee154etimer_schedule(DURATION_rt7);
}

inline void activity_rie5() {
   // log the error
   openserial_printError(COMPONENT_IEEE802154E,
                         ERR_WDRADIOTX_OVERFLOWS,
                         ieee154e_vars.state,
                         ieee154e_vars.asn%SCHEDULELENGTH);

   // abort
   endSlot();
}

inline void activity_ri8(uint16_t capturedTime) {
   // change state
   change_state(S_TXACK);

   // cancel rt7
   ieee154etimer_cancel();

   // record the captured time
   ieee154e_vars.capturedTime = capturedTime;

   // arm rt8
   ieee154etimer_schedule(DURATION_rt8);
}

inline void activity_rie6() {
   // log the error
   openserial_printError(COMPONENT_IEEE802154E,
                         ERR_WDACKDURATION_OVERFLOWS,
                         ieee154e_vars.state,
                         ieee154e_vars.asn%SCHEDULELENGTH);

   // abort
   endSlot();
}

inline void activity_ri9(uint16_t capturedTime) {
   // change state
   change_state(S_RXPROC);

   // cancel rt8
   ieee154etimer_cancel();

   // record the captured time
   ieee154e_vars.capturedTime = capturedTime;

   // free the ack we just sent so corresponding RAM memory can be recycled
   openqueue_freePacketBuffer(ieee154e_vars.ackToSend);
   
   // inform upper layer of reception
   res_receive(ieee154e_vars.dataReceived);

   // clear local variable
   ieee154e_vars.dataReceived = NULL;

   // official end of Rx slot
   endSlot();
}

//=========================== private =========================================

/**
\brief Calculates the frequency to transmit on, based on the 
absolute slot number and the channel offset of the requested slot.

\param [in] asn Absolute Slot Number
\param [in] channelOffset channel offset for the current slot

\returns The calculated frequency channel, an integer between 11 and 26.
*/
inline uint8_t calculateFrequency(asn_t asn, uint8_t channelOffset) {
   //return 11+(asn+channelOffset)%16;
   return 26;//poipoi
}

/**
\brief Decides whether the ack the mote just received is valid

\param [in] ackFrame The ack packet just received

\returns TRUE if ACK valid, FALSE otherwise
*/
bool isAckValid(OpenQueueEntry_t* ackFrame) {
   // TODO: implement
   return TRUE;
}

/**
\brief Decides whether the data frame the mote just received is valid

\param [in] dataFrame The data packet just received

\returns TRUE if data frame valid, FALSE otherwise
*/
bool isDataValid(OpenQueueEntry_t* dataFrame) {
   // TODO: implement
   return TRUE;
}

/**
\brief Decides whether the data just received needs to be acknowledged

\param [in] frame The data frame just received

\returns TRUE if acknowledgment is needed, FALSE otherwise
*/
bool ackRequested(OpenQueueEntry_t* frame) {
   // TODO: implement
   return TRUE;
}

/**
\brief Turns an newly reserved OpenQueueEntry_t in an ACK packet.

\param [out] frame The frame to turn into an ACK.
*/
void createAck(OpenQueueEntry_t* frame) {
   // TODO: implement
   return;
}

/**
\brief Changes the state of the IEEE802.15.4e FSM.

Besides simply updating the state global variable,
this function toggles the FSM debug pin.

\param [in] newstate The state the IEEE802.15.4e FSM is now in.
*/
void change_state(uint8_t newstate) {
   // update the state
   ieee154e_vars.state = newstate;
   // wiggle the FSM debug pin
   switch (ieee154e_vars.state) {
      case S_SYNCHRONIZING:
      case S_TXDATAOFFSET:
         DEBUG_PIN_FSM_SET();
         break;
      case S_SLEEP:
      case S_RXDATAOFFSET:
         DEBUG_PIN_FSM_CLR();
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

/**
\brief Housekeeping tasks to do at the end of each slot.

This functions is called once in each slot, when there is nothing more
to do. This might be when an error occured, or when everything went well.
This function resets the state of the FSM so it is ready for the next slot.

Note that by the time this function is called, any received packet should already
have been sent to the upper layer. Similarly, in a Tx slot, the sendDone
function should already have been done. If this is not the case, this function
will do that for you, but assume that something went wrong.
*/
void endSlot() {
   // turn off the radio
   radio_rfOff();
   
   // clear any pending timer
   ieee154etimer_cancel();

   // reset capturedTime
   ieee154e_vars.capturedTime = 0;

   // clean up dataToSend
   if (ieee154e_vars.dataToSend!=NULL) {
      // if everything went well, dataToSend was set to NULL in ti9
      // transmit failed, decrement transmits left counter
      ieee154e_vars.dataToSend->l2_retriesLeft--;
      // indicate tx fail if no more retries left
      if (ieee154e_vars.dataToSend->l2_retriesLeft==0) {
         res_sendDone(ieee154e_vars.dataToSend,E_FAIL);
      }
      // reset local variable
      ieee154e_vars.dataToSend = NULL;
   }

   // clean up dataReceived
   if (ieee154e_vars.dataReceived!=NULL) {
      // assume something went wrong. If everything went well, dataReceived
      // would have been set to NULL in ri9.
      // indicate  "received packet" to upper layer since we don't want to loose packets
      res_receive(ieee154e_vars.dataReceived);
      // reset local variable
      ieee154e_vars.dataReceived = NULL;
   }

   // clean up ackToSend
   if (ieee154e_vars.ackToSend!=NULL) {
      // free ackToSend so corresponding RAM memory can be recycled
      openqueue_freePacketBuffer(ieee154e_vars.ackToSend);
      // reset local variable
      ieee154e_vars.ackToSend = NULL;
   }

   // clean up ackReceived
   if (ieee154e_vars.ackReceived!=NULL) {
      // free ackReceived so corresponding RAM memory can be recycled
      openqueue_freePacketBuffer(ieee154e_vars.ackReceived);
      // reset local variable
      ieee154e_vars.ackReceived = NULL;
   }
   
   // change state
   change_state(S_SLEEP);
}