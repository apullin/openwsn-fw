/*
 * TSCH on OpenWSN
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 * Branko Kerkez   <bkerkeze@berkeley.edu>, March 2011
 */
#include "openwsn.h"
#include "IEEE802154E.h"
#include "IEEE802154.h"
#include "radio.h"
#include "packetfunctions.h"
#include "idmanager.h"
#include "openserial.h"
#include "openqueue.h"
#include "timers.h"
#include "packetfunctions.h"
#include "neighbors.h"
#include "nores.h"


//for debugging and hardocing to test synchronization
#define MOTE1_ADDRESS   0x87
#define MOTE2_ADDRESS   0xb6

//===================================== variables ==============================

uint16_t           fastAlarmStartSlotTimestamp;
uint16_t           slotAlarmStartSlotTimestamp;
uint8_t            state;
asn_t              asn;              //uint16_t  
OpenQueueEntry_t*  dataFrameToSend; //NULL at beginning and end
OpenQueueEntry_t*  packetACK;       //NULL at beginning and end, free at end of slot
OpenQueueEntry_t*  frameReceived;   //NULL at beginning and end
bool               isSync;
uint8_t            dsn;
uint8_t            frequencyChannel;
error_t            sendDoneError;

//===================================== prototypes =============================

#include "IEEE802154_common.c"
void       change_state(uint8_t newstate);
void       resynchronize(bool resyncType, open_addr_t* node_id, timervalue_t dataGlobalSlotOffset, int16_t timeCorrection);
void       fast_alarm_fired();
void       endSlot();
void       taskDebugPrint();             //task
bool       mac_debugPrint();
void       taskResetLosingLostTimers();  //task
uint16_t   fastAlarm_getNow();
uint16_t   slotAlarm_getNow();
//void fastAlarm_start(uint16_t duration);
//void slotAlarm_startAt(uint16_t startingPoint,uint16_t duration); 
//void fastAlarm_startAt(uint16_t startingPoint,uint16_t duration);

void notifyReceive();
void notifySendDone(OpenQueueEntry_t* param_sendDoneMessage, error_t param_sendDoneError);

//the following are prototypes for cellUsage -- which needs to be moved to a separate library
uint8_t cellUsageGet_getType(uint16_t slotNum);
uint8_t cellUsageGet_isTX(uint16_t slotNum);
uint8_t cellUsageGet_isRX(uint16_t slotNum);
uint8_t cellUsageGet_isADV(uint16_t slotNum);
uint8_t cellUsageGet_getChannelOffset(uint16_t slotNum);

void prepareADVPacket();

//===================================== public from upper layer ===============

void mac_init() {    
   //set debug pins as outputs
   DEBUG_PIN_FRAME_OUT; //P1.1 0x02
   DEBUG_PIN_SLOT_OUT;  //P4.4 0x04
   DEBUG_PIN_FAST_OUT;  //P4.3 0x08
   
   isSync          = 0; 
   dataFrameToSend = NULL;
   asn             = 0;
   change_state(S_SLEEP);
   
   /*hardcoded poipoi
   we need to make one mote the DAG root for synchronization purposes
   make mote1 the DAG root*/
   if(*(&eui64+7) == MOTE1_ADDRESS) {
      idmanager_setIsDAGroot(TRUE);
   }
   
   //Timer B5 is used in capture mode to timestemp incoming radio packets
   enable_capture(TIMER_B5);
   timer_startPeriodic(TIMER_MAC_PERIODIC,PERIODICTIMERPERIOD);
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

//===================================SLOT TX/RX================================

//new slot
void timer_mac_periodic_fired() {
   ieee802154_header_iht   transmitted_ieee154_header;
   uint8_t                 channelOffset;
   
   /*This is an example for a 10ms timeslot
   After the slot fires, we start a backoff timer
   if we are a transmitter, we have already loaded a packet into the radio buffer, and we simply tranmitt when the timer fires (say 2ms)
   as a receiver, we want to extend the timer (by 1ms, for exmaple) so we can listen to an incoming packet,
   which will hopefully have been sent at 2ms
   */
   //timer_startOneShot(TIMER_MAC_BACKOFF,MINBACKOFF);
   //fastAlarmStartSlotTimestamp     = fastAlarm_getNow();
   //slotAlarmStartSlotTimestamp     = slotAlarm_getNow();
   
   //increment the absolute slot number
   asn++;
   

   //set fast_alarm debug pin to 0 for easier debugging
   DEBUG_PIN_SLOT_TOGGLE;
   //flip frame debug pin
   if(asn%LENGTHCELLFRAME == 0) {
      DEBUG_PIN_FRAME_TOGGLE;
   }
   
   openserial_stop();
   
   dataFrameToSend = NULL;
   
   //----- switch to/from S_SYNCHRONIZING
   if (idmanager_getIsDAGroot()==TRUE) {
      //if I'm DAGroot, I'm synchronized
      taskResetLosingLostTimers();
      isSync=TRUE;
      if (state==S_SYNCHRONIZING) { //happens right after node becomes LBR
         endSlot();
         return;
      }
   } else if (isSync==FALSE) {
      //If I'm not in sync, enter/stay in S_SYNCHRONIZING
      /*atomic if (asn%2==1) {
      openserial_startOutput();
   } else {
      openserial_startInput();
   }*///poipoi
      if (state!=S_SYNCHRONIZING) {
         change_state(S_SYNCHRONIZING);
         radio_rxOn(18);
      }
      return;
   }
   
   //----- state error
   if (state!=S_SLEEP) {     
      openserial_printError(COMPONENT_MAC,ERR_WRONG_STATE_IN_STARTSLOTTASK,
                            (errorparameter_t)state,(errorparameter_t)asn%LENGTHCELLFRAME);
      endSlot();
      return;
   }
   
   switch (cellUsageGet_getType(asn%LENGTHCELLFRAME)) {
   case CELLTYPE_TXRX:
      
      //start timer to deal with transmitting or receiving when backoff timer fires
      timer_startOneShot(TIMER_MAC_BACKOFF,MINBACKOFF);//set timer to deal with TXRX in this slot
      
      // poipoipoi start =====================================
      OpenQueueEntry_t* packetADV;
      //get a packet out of the buffer (if any)
      if (cellUsageGet_isTX(asn%LENGTHCELLFRAME)){//poipoi || cellUsageGet_isSH_TX(asn%LENGTHCELLFRAME)) {
         //hack add adv packet to queue
         if(cellUsageGet_isADV(asn%LENGTHCELLFRAME)){
            packetADV = openqueue_getFreePacketBuffer();
            if (packetADV==NULL) {
               P1OUT |= 0x04;
            } else {
               packetADV->creator       = COMPONENT_RES;
               packetADV->owner         = COMPONENT_MAC;
               packetfunctions_reserveHeaderSize(packetADV,sizeof(IEEE802154E_ADV_t));
               ((IEEE802154E_ADV_t*)(packetADV->payload))->commandFrameId=IEEE154E_ADV;
               prependIEEE802154header(packetADV,
                                       IEEE154_TYPE_CMD,
                                       IEEE154_SEC_NO_SECURITY,
                                       NULL,
                                       NULL);
               dataFrameToSend = packetADV; 
            }
         }
      }
      // poipoipoi stop ======================================
      
      channelOffset = cellUsageGet_getChannelOffset(asn%LENGTHCELLFRAME);
      if (HOPPING_ENABLED) {
         frequencyChannel = ((asn+channelOffset)%16)+11;
      } else {
         frequencyChannel = ((    channelOffset)%16)+11;
      }
      
      frequencyChannel=18; //poipoi change
      
      if (dataFrameToSend!=NULL) {                                   // start the TX sequence
         dataFrameToSend->owner      = COMPONENT_MAC;
         dataFrameToSend->l1_channel = frequencyChannel;
         transmitted_ieee154_header  = retrieveIEEE802154header(dataFrameToSend);
         if (cellUsageGet_isADV(asn%LENGTHCELLFRAME)) {
            //I will be sending an ADV frame
            ((IEEE802154E_ADV_t*)((dataFrameToSend->payload)+transmitted_ieee154_header.headerLength))->timingInformation=asn;// (globalTime.getASN());//poipoi implement
         }
         change_state(S_TX_TXDATAPREPARE);
         if (radio_prepare_send(dataFrameToSend)!=E_SUCCESS) {
            //retry sending the packet later
            dataFrameToSend->l2_retriesLeft--;
            if (dataFrameToSend->l2_retriesLeft==0) {
               notifySendDone(dataFrameToSend,FAIL);
            }
            openserial_printError(COMPONENT_MAC,ERR_PREPARESEND_FAILED,
                                  (errorparameter_t)asn%LENGTHCELLFRAME,(errorparameter_t)0);
            endSlot();
         };
         //fastAlarm_startAt(fastAlarmStartSlotTimestamp,TsTxOffset);//watchdog timer
      } else {
         if (cellUsageGet_isRX(asn%LENGTHCELLFRAME)) {        //start the RX sequence
            change_state(S_RX_RXDATAPREPARE);
            radio_rxOn(frequencyChannel);
            /*
            if (radio_rxOn(frequencyChannel)!=E_SUCCES) {
            //abort
            openserial_printError(COMPONENT_MAC,ERR_PREPARERECEIVE_FAILED,
            (errorparameter_t)asn%LENGTHCELLFRAME,(errorparameter_t)0);
            endSlot();
            };
            */
            
            //add some time to extend wait period for incoming packet to allow for a little overlap
            //TBCCR1 is used by TIMER_MAC_BACKOFF
            TBCCR1   = TBCCR1+EXTRA_WAIT_TIME;   //add one extra guardtime to wait for incoming packet
            TBCCTL1  = CCIE;                     //enable interupt
            
            //fastAlarm_startAt(fastAlarmStartSlotTimestamp,TsRxOffset);
         } else {                                                    // nothing to do, abort
            openserial_startOutput();
            endSlot();
            return;
         }
      }
      break;
      
   case CELLTYPE_RXSERIAL:
      openserial_startInput();
      endSlot();
      return;
      
   case CELLTYPE_OFF:
      openserial_startOutput();
      endSlot();
      return;
      
   default:
      openserial_printError(COMPONENT_MAC,ERR_WRONG_CELLTYPE,
                            cellUsageGet_getType(asn%LENGTHCELLFRAME),0);
      endSlot();
      return;
   }
}

void radio_prepare_send_done(){//poipoi implememnt in phys layer
   switch (state) {
   case S_TX_TXDATAPREPARE:
      change_state(S_TX_TXDATAREADY);//bk added
      break;
   case S_RX_TXACKPREPARE:
      change_state(S_RX_TXACKREADY);
      break;
   default:
      openserial_printError(COMPONENT_MAC,ERR_WRONG_STATE_IN_PREPARESENDDONE,
                            (errorparameter_t)state,(errorparameter_t)asn%LENGTHCELLFRAME);
      endSlot();
      break;
   }
}

void radioControl_prepareReceiveDone(error_t error) {
   if (error!=E_SUCCESS) {
      //abort
      openserial_printError(COMPONENT_MAC,ERR_PREPARERECEIVEDONE_FAILED,
                            (errorparameter_t)error,(errorparameter_t)asn%LENGTHCELLFRAME);
      endSlot();
   }
   switch (state) {
      case S_TX_RXACKPREPARE:
         change_state(S_TX_RXACKREADY);
         break;
      case S_RX_RXDATAPREPARE:
         change_state(S_RX_RXDATAREADY);
         break;
      case S_SYNCHRONIZING:
         //if ( (RadioControl.receiveNow(TIME_LIMITED_RX,TsRxWaitTime))!=SUCCESS ) { poipoi make so radio only stays on for some time
         radio_rxOn(frequencyChannel);   
         /*
         if(radio_rxOn(frequencyChannel)!=E_SUCCESS){
         //abort
         openserial_printError(COMPONENT_MAC,ERR_RECEIVENOW_FAILED,
         (errorparameter_t)state,(errorparameter_t)asn%LENGTHCELLFRAME);
         endSlot();
         }poipoi
         */
         break;
      default:
         openserial_printError(COMPONENT_MAC,ERR_WRONG_STATE_IN_PREPARERECEIVEDONE,
                               (errorparameter_t)state,(errorparameter_t)asn%LENGTHCELLFRAME);
         endSlot();
         break;
   }
}

//wrappers - -this is a bit janky, but this way we can just repalce this ...
//...tsch MAC with StupidMac without ediding the  timer and scheduler files
void timer_mac_backoff_fired(){
   fast_alarm_fired();//see next
}

void timer_mac_watchdog_fired(){
   fast_alarm_fired();//see next
}

//end wrapers

void fast_alarm_fired() {
   
   switch (state) {
      /*------------------- TX sequence ------------------------*/
      case S_TX_TXDATAPREPARE:                                    //[timer fired] transmitter (ERROR state)
         //I'm a transmitter, didn't have time to prepare for TX
         notifySendDone(dataFrameToSend,FAIL);
         openserial_printError(COMPONENT_MAC,ERR_NO_TIME_TO_PREPARE_TX,
                               (errorparameter_t)asn%LENGTHCELLFRAME,0);
         endSlot();
         break;
      case S_TX_TXDATAREADY:                                      //[timer fired] transmitter
         //I'm a transmitter, Tx data now
         change_state(S_TX_TXDATA);
         if ((radio_send_now())!=E_SUCCESS) {
            //retry later
            dataFrameToSend->l2_retriesLeft--;
            if (dataFrameToSend->l2_retriesLeft==0) {
               notifySendDone(dataFrameToSend,FAIL);
            }
            openserial_printError(COMPONENT_MAC,ERR_SENDNOW_FAILED,
                                  (errorparameter_t)state,(errorparameter_t)asn%LENGTHCELLFRAME);
            endSlot();
         } else {
            //if we are sucessful start the timer to turn the radio on later to listen for th eack
            //no need to listen if we just sent an ADV (slot == 0)
            if(asn%LENGTHCELLFRAME != 0)
               timer_startOneShot(TIMER_MAC_WATCHDOG,GUARDTIME);
         }
         break;
      case S_TX_RXACKREADY:                                       //[timer fired] transmitter
         //I'm a transmitter, switch on RX for ACK now
         //this is done automatically after Tx is finished, rx is already on.
         //I'm calling receiveNow anyways because it reports receivedNothing
         change_state(S_TX_RXACK);
         radio_rxOn(frequencyChannel);
         //start timer to listen only for a while
         //if we dont get packet by ACK_WAIT_TIME+, we kill the receiver
         timer_startOneShot(TIMER_MAC_BACKOFF,ACK_WAIT_TIME+EXTRA_WAIT_TIME);
         
         
         /*
         if ( radio_rxOn(frequencyChannel)!=E_SUCCESS ) {
         //abort
         openserial_printError(COMPONENT_MAC,ERR_RECEIVENOW_FAILED,
         (errorparameter_t)state,(errorparameter_t)0);
         endSlot();
         };poipoi
         */
         break;//from here***
         
      /*------------------- RX sequence -----------------------*/
      case S_RX_RXDATAPREPARE:                                    //[timer fired] receiver (ERROR state)
         //I'm a receiver, didn't have time to prepare for RX
         openserial_printError(COMPONENT_MAC,ERR_NO_TIME_TO_PREPARE_RX,
                               (errorparameter_t)asn%LENGTHCELLFRAME,0);
         endSlot();
         break;
      case S_RX_RXDATAREADY:                                      //[timer fired] receiver
         //I'm a receiver, switch RX radio on for data now
         change_state(S_RX_RXDATA);
         radio_rxOn(frequencyChannel);
         /*
         if ( (call RadioControl.receiveNow(TIME_LIMITED_RX,TsRxWaitTime))!=SUCCESS ) {
         //abort
         openserial_printError(COMPONENT_MAC,ERR_RECEIVENOW_FAILED,
         (errorparameter_t)state,(errorparameter_t)0);
         endSlot();
         };poipoi
         */
         break;
      case S_RX_TXACKPREPARE:                                     //[timer fired] receiver (ERROR state)
         //I'm a receiver, didn't have time to prepare ACK
         openserial_printError(COMPONENT_MAC,ERR_NO_TIME_TO_PREPARE_ACK,
                               (errorparameter_t)asn%LENGTHCELLFRAME,0);
         endSlot();
         break;
      case S_RX_TXACKREADY:                                       //[timer fired] receiver
         //I'm a receiver, TX ACK now
         change_state(S_RX_TXACK);
         radio_send_now();
         /*
         if ((radio_send_now()) {
         //abort
         openserial_printError(COMPONENT_MAC,ERR_SENDNOW_FAILED,
         (errorparameter_t)state,(errorparameter_t)asn%LENGTHCELLFRAME);
         endSlot();
         }
         */
         break;
         
      default:
         openserial_printError(COMPONENT_MAC,ERR_WRONG_STATE_IN_FASTTIMER_FIRED,
                               (errorparameter_t)state,(errorparameter_t)asn%LENGTHCELLFRAME);
         endSlot();
         break;
      }
}

//call in mac, not from radio, if no interupt by now, turn radio off
void radioControl_receivedNothing(error_t error){
   switch(state) {
      case S_RX_RXDATA:                                           //[receivedNothing] receiver (WARNING state)
         //I'm a receiver, didn't receive data
         endSlot();
         break;
      case S_TX_RXACK:                                            //[receivedNothing] transmitter (WARNING state)
         //I'm a transmitter, didn't receive ACK (end of TX sequence)
         
         /// major poipoi, implement both of these later for true tsch neighbor management
         //cellStats_indicateUse(asn%LENGTHCELLFRAME,WAS_NOT_ACKED);
         //count num of sent packets
         //neighborStats_indicateTx(&(dataFrameToSend->l2_nextORpreviousHop),WAS_NOT_ACKED);
         
         dataFrameToSend->l2_retriesLeft--;
         if (dataFrameToSend->l2_retriesLeft==0) {
            notifySendDone(dataFrameToSend,FAIL);
         }
         endSlot();
         break;
      case S_SYNCHRONIZING:                                       //[receivedNothing] synchronizer
         //it's OK not to receive anything after TsRxWaitTime when trying to synchronize
         break;
      default:
         openserial_printError(COMPONENT_MAC,ERR_WRONG_STATE_IN_RECEIVEDNOTHING,
                               (errorparameter_t)state,(errorparameter_t)asn%LENGTHCELLFRAME);
         endSlot();
         break;
   } 
}

void radio_send_now_done(error_t error) {//poipoi call from phy layer   
   switch (state) {
      case S_TX_TXDATA:                                           //[sendNowDone] transmitter
         //I'm a transmitter, finished sending data
         if (error!=E_SUCCESS) {
            //retry later
            dataFrameToSend->l2_retriesLeft--;
            if (dataFrameToSend->l2_retriesLeft==0) {
               notifySendDone(dataFrameToSend,FAIL);
            }
            openserial_printError(COMPONENT_MAC,ERR_SENDNOWDONE_FAILED,
                                  (errorparameter_t)state,
                                  (errorparameter_t)asn%LENGTHCELLFRAME);
            endSlot();
            return;
         }
         if (cellUsageGet_isADV(asn%LENGTHCELLFRAME)==TRUE) {
            //ADV slot, don't have to listen for ACK
            /// major poipoi, implement all three of these later for true tsch neighbor management
            //call NeighborStats.indicateTx(&(dataFrameToSend->l2_nextORpreviousHop),WAS_NOT_ACKED);
            //call CellStats.indicateUse(asn%LENGTHCELLFRAME,WAS_NOT_ACKED);
            notifySendDone(dataFrameToSend,E_SUCCESS);
            
            endSlot();
         } else {
            //poipoi removed for debugging 
            //fastAlarm_start(TsRxAckDelay);
            change_state(S_TX_RXACKREADY);
         }
         break;
      case S_RX_TXACK:                                            //[sendNowDone] receiver
         //I'm a receiver, finished sending ACK (end of RX sequence)
         if (error!=E_SUCCESS) {
            //don't do anything if error==FAIL
            openserial_printError(COMPONENT_MAC,ERR_SENDNOWDONE_FAILED,
                                  (errorparameter_t)state,
                                  (errorparameter_t)asn%LENGTHCELLFRAME);
         }
         /* //sync off of DATA I received before I sent ACK
         * poipoi for simplicity, only resync from ADV
         resynchronize(FRAME_BASED_RESYNC,
         &(frameReceived->l2_nextORpreviousHop),
         frameReceived->l1_rxTimestamp,
         (int16_t)((int32_t)(frameReceived->l1_rxTimestamp)-(int32_t)radio_delay)-(int32_t)TsTxOffset);*/
         
         notifyReceive(); 
         endSlot();
         break;
      default:
         openserial_printError(COMPONENT_MAC,ERR_WRONG_STATE_IN_SUBSEND_SENDDONE,
                               (errorparameter_t)state,(errorparameter_t)asn%LENGTHCELLFRAME);
         endSlot();
         break;
   }
}

void notifySendDone(OpenQueueEntry_t* param_sendDoneMessage, error_t param_sendDoneError) {
   if (param_sendDoneMessage->creator==COMPONENT_RES) {
      openqueue_freePacketBuffer(param_sendDoneMessage);
   } else {
      nores_sendDone(param_sendDoneMessage,param_sendDoneError);
   }
}

void endSlot() {
   //if (packetACK!=NULL) {
   //malloc_freePacketBuffer(packetACK);
   ///      openqueue_freePacketBuffer(packetACK);
   //      packetACK=NULL;
   //}
   //radio_rfOff();
   /*
   if (radio_rfOff()!=E_SUCCESS) {
   //abort
   openserial_printError(COMPONENT_MAC,ERR_RFOFF_FAILED,
   (errorparameter_t)state,(errorparameter_t)asn%LENGTHCELLFRAME);
   }
   */
   if (state!=S_SYNCHRONIZING) {
      change_state(S_SLEEP);
   }
   CLEAR_TIMER_B5_OVERFLOW();    //need to clear timer capture
}

//===================================== public from lower layer ===============

void mac_sendDone(OpenQueueEntry_t* pkt, error_t error) {
   notifySendDone(pkt, error);
}

//===================================== private ===============================

void radio_packet_received(OpenQueueEntry_t* msg) {
   ieee802154_header_iht   received_ieee154_header;
   ieee802154_header_iht   transmitted_ieee154_header;
   
   msg->owner = COMPONENT_MAC;
   
   received_ieee154_header = retrieveIEEE802154header(msg);
   packetfunctions_tossHeader(msg,received_ieee154_header.headerLength);
   packetfunctions_tossFooter(msg,2);
   
   msg->l2_frameType = received_ieee154_header.frameType;
   memcpy(&(msg->l2_nextORpreviousHop),&(received_ieee154_header.src),sizeof(open_addr_t));
   
   if (received_ieee154_header.frameType==IEEE154_TYPE_DATA &&
       !(idmanager_isMyAddress(&received_ieee154_header.panid))) {
          openserial_printError(COMPONENT_MAC,ERR_WRONG_PANID,
                                (errorparameter_t)received_ieee154_header.panid.panid[0]*256+received_ieee154_header.panid.panid[1],
                                (errorparameter_t)0);
          openqueue_freePacketBuffer(msg);
          return;
       }
   
   switch (state) {
      /*------------------- TX sequence ------------------------*/
      case S_TX_RXACK:                                            //[receive] transmitter
         //I'm a transmitter, just received ACK (end of TX sequence)
         transmitted_ieee154_header = retrieveIEEE802154header(dataFrameToSend);
         if (received_ieee154_header.dsn == transmitted_ieee154_header.dsn) {
            //I'm a transmitter, sync off of ACK message
            /* poipoi for simplicity, only resync from ADV
            resynchronize(ACK_BASED_RESYNC,
            &received_ieee154_header.src,
            TsTxOffset,
            (((IEEE802154E_ACK_ht*)(msg->payload))->timeCorrection));//poipoi /32  poipoipoipoi*/
            
            //poipoi implement
            //cellStats_indicateUse(asn%LENGTHCELLFRAME,WAS_ACKED);
            //neighborStats_indicateTx(&(dataFrameToSend->l2_nextORpreviousHop),WAS_ACKED);
            
            //notifySendDone(dataFrameToSend,SUCCESS); poipoi what?
         } else {
            //implement these poipoi
            // cellStats.indicateUse(asn%LENGTHCELLFRAME,WAS_NOT_ACKED);
            //  neighborStats.indicateTx(&(dataFrameToSend->l2_nextORpreviousHop),WAS_NOT_ACKED);
            dataFrameToSend->l2_retriesLeft--;
            if (dataFrameToSend->l2_retriesLeft==0) {
               notifySendDone(dataFrameToSend,FAIL);
            }
         }
         openqueue_freePacketBuffer(msg);//free ACK
         endSlot();
         break;
         
         /*------------------- RX sequence ------------------------*/
      case S_SYNCHRONIZING:
      case S_RX_RXDATA:                                           //[receive] receiver
         //I'm a receiver, just received data
         if (idmanager_isMyAddress(&(received_ieee154_header.dest)) && received_ieee154_header.ackRequested) {
            //ACK requested
            radio_rfOff();
            /*if (radio_rfOff()!=E_SUCCESS) {
            //do nothing about it
            openserial_printError(COMPONENT_MAC,ERR_RFOFF_FAILED,
            (errorparameter_t)state,(errorparameter_t)asn%LENGTHCELLFRAME);
         }*/
            
            //poipoi add for ack
            //fastAlarm_start(TsTxAckDelay);
            //change_state(S_RX_TXACKPREPARE);
            
            packetACK = openqueue_getFreePacketBuffer();
            
            if (packetACK==NULL) {
               openserial_printError(COMPONENT_MAC,ERR_NO_FREE_PACKET_BUFFER,
                                     (errorparameter_t)0,(errorparameter_t)0);
               openqueue_freePacketBuffer(msg);
               endSlot();
               return;
            }
            packetACK->creator       = COMPONENT_MAC;
            packetACK->owner         = COMPONENT_MAC;
            //ACK payload
            packetfunctions_reserveHeaderSize(packetACK,sizeof(IEEE802154E_ACK_ht));
            ((IEEE802154E_ACK_ht*)(packetACK->payload))->dhrAckNack     = IEEE154E_ACK_dhrAckNack_DEFAULT;
            ((IEEE802154E_ACK_ht*)(packetACK->payload))->timeCorrection =
               (int16_t)((int32_t)(TsTxOffset+radio_delay)-(int32_t)(msg->l1_rxTimestamp));//poipoi *32
              //154 header
              prependIEEE802154header(packetACK,
                                      IEEE154_TYPE_ACK,
                                      IEEE154_SEC_NO_SECURITY,
                                      received_ieee154_header.dsn,
                                      NULL
                                         );
              //l2 metadata
              packetACK->l2_retriesLeft  = 1;
              //l1_metadata
              packetACK->l1_txPower        = TX_POWER;
              packetACK->l1_channel = frequencyChannel;
              error_t error = radio_prepare_send(packetACK);
              
              //now that we have an ACK packet we want to send it when ACK_WAIT_TIME fires
              timer_startOneShot(TIMER_MAC_BACKOFF,ACK_WAIT_TIME);
              
              if (error!=E_SUCCESS) {
                 //abort
                 openserial_printError(COMPONENT_MAC,ERR_PREPARESEND_FAILED,
                                       (errorparameter_t)asn%LENGTHCELLFRAME,(errorparameter_t)0);
                 endSlot();
              };
              frameReceived = msg;
         } else if (packetfunctions_isBroadcastMulticast(&(received_ieee154_header.dest))) {
            //I'm a receiver, sync off of DATA I received iif ADV (I will not send an ACK)
            if (received_ieee154_header.frameType==IEEE154_TYPE_CMD &&
                ((IEEE802154E_ADV_t*)(msg->payload))->commandFrameId==IEEE154E_ADV) {
                   asn = ((IEEE802154E_ADV_t*)(msg->payload))->timingInformation;
                   resynchronize(FRAME_BASED_RESYNC,
                                 &received_ieee154_header.src,
                                 msg->l1_rxTimestamp,
                                 (int16_t)((int32_t)(msg->l1_rxTimestamp)-(int32_t)radio_delay)-(int32_t)TsTxOffset);
                }
            frameReceived = msg;
            notifyReceive();
            endSlot();
         } else{
            openqueue_freePacketBuffer(msg);
            endSlot();
         }
         break;
         
      default:
         openqueue_freePacketBuffer(msg);
         endSlot();
         openserial_printError(COMPONENT_MAC,ERR_WRONG_STATE_IN_RECEIVE,
                               (errorparameter_t)state,(errorparameter_t)asn%LENGTHCELLFRAME);
         endSlot();
         break;
   }
}

void notifyReceive() { 
   if (frameReceived->length>0) {
      //packet contains payload destined to an upper layer
      nores_receive(frameReceived);
   } else {
      openqueue_freePacketBuffer(frameReceived);
   }
   frameReceived = NULL;
}

/*------------------------------ resynchronization ---------------------------------*/
void resynchronize(bool resyncType, open_addr_t* node_id, timervalue_t dataGlobalSlotOffset, int16_t timeCorrection) {   
   //open_addr_t   timeParent;
   bool          iShouldSynchronize;
   
   if ((idmanager_getIsDAGroot())==FALSE) {        //I resync only if I'm not a DAGroot
      //---checking whether I should synchronize
      iShouldSynchronize=FALSE;
      if (isSync==FALSE) {                    //I'm not synchronized, I sync off of all ADV packets
         if (resyncType==FRAME_BASED_RESYNC) {
            iShouldSynchronize=TRUE;
            isSync=TRUE;
            openserial_printError(COMPONENT_MAC,ERR_ACQUIRED_SYNC,0,0);//not an error!
            //call Leds.led2On(); poipoi
         }
      } else {                                //I'm already synchronized
         //implement poipoi
         /*
         neighborGet_getPreferredParent(&timeParent,node_id->type);
         if (timeParent.type!=ADDR_NONE) {  //I have a timeparent, I sync off of any packet from it
         if (packetfunctions_sameAddress(&timeParent,node_id)) {
         iShouldSynchronize=TRUE;
         }
         } else {                        //I don't have a timeparent, I sync off of any packet
            iShouldSynchronize=TRUE;
         }end implement
         */
      }
      //---synchronize iif I need to
      if (iShouldSynchronize==TRUE) {
         if (resyncType==FRAME_BASED_RESYNC) {
            if (dataGlobalSlotOffset!=INVALID_TIMESTAMP){
               if (slotAlarmStartSlotTimestamp+dataGlobalSlotOffset<slotAlarm_getNow()) {
                  //slotAlarm_startAt((uint32_t)((int32_t)slotAlarmStartSlotTimestamp+(int32_t)timeCorrection),SLOT_TIME);
               } else {
                  isSync=FALSE;
                  openserial_printError(COMPONENT_MAC,ERR_SYNC_RACE_CONDITION,
                                        (errorparameter_t)asn%LENGTHCELLFRAME,
                                        (errorparameter_t)dataGlobalSlotOffset);
                  //call Leds.led2Off(); poipoi
                  // slotAlarm_startAt((slotAlarm_getNow())-(SLOT_TIME/2),SLOT_TIME);
                  endSlot();
               }
            }
         } else {
            //slotAlarm_startAt((uint32_t)((int32_t)slotAlarmStartSlotTimestamp+(int32_t)timeCorrection),SLOT_TIME);
         }
         taskResetLosingLostTimers();
      }
   }
}

/*------------------------------ misc ----------------------------------------------*/

void taskResetLosingLostTimers() {
   //losingSyncTimer_startOneShot(DELAY_LOSING_NEIGHBOR_1KHZ);
   //lostSyncTimer_startOneShot(DELAY_LOST_NEIGHBOR_1KHZ);
}

void taskDebugPrint() {
   //nothing to output
}

//SoftwareInit
error_t softwareInit_init() {//poipoi
   change_state(S_SLEEP);
   dataFrameToSend = NULL;
   asn = 0;
   //WDT configuration
   //WDTCTL = WDTPW + WDTHOLD;
   //WDTCTL = WDTPW + WDTCNTCL + WDTSSEL;//run from ACLK, ~1s
   return E_SUCCESS;
}

//LosingSyncTimer
void losingSyncTimer_fired() {
   //globalSync_losingSync(); poipoi
   openserial_printError(COMPONENT_MAC,ERR_LOSING_SYNC,0,0);
}

//LostSyncTimer
void LostSyncTimer_fired() {
   openserial_printError(COMPONENT_MAC,ERR_LOST_SYNC,0,0);
   isSync=FALSE;
   //call Leds.led2Off(); poipoi
   //globalSync_lostSync(); poipoi
}

//GlobalTime
//doo we really need this
/*timervalue_t GlobalTime_getGlobalSlotOffset() {
//SlotAlarm.getNow() is epoch of now
//(SlotAlarm.getAlarm()-SLOT_TIME) is epoch of the start of the slot
//(call SlotAlarm.getNow())-(SlotAlarm.getAlarm()-SLOT_TIME) is the time since start of cell
return ((slotAlarm_getNow())-((slotAlarm_getAlarm())-(uint32_t)SLOT_TIME));
   }*/

timervalue_t GlobalTime_getLocalTime() {
   return (slotAlarm_getNow());
}

//asn_t GlobalTime.getASN() {
//   return asn;
//}

bool globalSync_getIsSync() {
   return isSync;
}

void debugPrint_print() {
   taskDebugPrint();
}

void change_state(uint8_t newstate) {
   state = newstate;
   switch (newstate) {
   case S_SYNCHRONIZING:
   case S_TX_TXDATA:
   case S_TX_RXACK:
   case S_RX_RXDATA:
   case S_RX_TXACK:
      //call Port35.set();
      //call Leds.led1On();
      // break;
   case S_TX_TXDATAPREPARE:
   case S_TX_TXDATAREADY:
   case S_TX_RXACKPREPARE:
   case S_TX_RXACKREADY:
   case S_RX_RXDATAPREPARE:
   case S_RX_RXDATAREADY:
   case S_RX_TXACKPREPARE:
   case S_RX_TXACKREADY:
   case S_SLEEP:
      //call Port35.clr();
      //call Leds.led1Off();
      break;
   }
}

uint16_t fastAlarm_getNow(){
   return 0;//poipoi use this to return the timer value
}

uint16_t slotAlarm_getNow(){
   return 0;//poipoi use this to return the timer value
}

/*void fastAlarm_start(uint16_t duration) {
timer_startOneShot(TIMER_MAC_BACKOFF,duration);
}

//no!
void fastAlarm_startAt(uint16_t startingPoint,uint16_t duration) {
for(int i =0; i< startingPoint; i++);//poipoi super hack fix this asap
timer_startOneShot(TIMER_MAC_BACKOFF,duration);
}

void slotAlarm_startAt(uint16_t startingPoint,uint16_t duration){
for(int i =0; i< startingPoint; i++);//poipoi super hack fix this asap
timer_startPeriodic(TIMER_MAC_PERIODIC,duration);
}*/


bool mac_debugPrint() {
   return FALSE;
}

//*********************************hardcore hack -- move later **********//
//*********************************cellUsage functions*******************//

uint8_t cellUsageGet_getType(uint16_t slotNum){
   if(slotNum < 3) {
      return CELLTYPE_TXRX;
   } else if(slotNum == 4) {
      return CELLTYPE_RXSERIAL;
   } else {
      return CELLTYPE_OFF;
   }
}

uint8_t cellUsageGet_isTX(uint16_t slotNum){
   //hardcoded
   if((slotNum == 0 && *(&eui64+7) == MOTE1_ADDRESS)||
      (slotNum == 1 && *(&eui64+7) == MOTE1_ADDRESS)||
      (slotNum == 2 && *(&eui64+7) == MOTE2_ADDRESS)) {
      return 1;
   } else {
      return 0;
   }
}

//b003a
///*(&eui64+7) gives you the last byte of the address of a gina
uint8_t cellUsageGet_isRX(uint16_t slotNum){
   //hardcoded
   if((slotNum == 0 && *(&eui64+7) == MOTE2_ADDRESS)||
      (slotNum == 1 && *(&eui64+7) == MOTE2_ADDRESS)||
      (slotNum == 2 && *(&eui64+7) == MOTE1_ADDRESS)) {
      return 1;
   } else {
      return 0; 
   }
}

uint8_t cellUsageGet_isADV(uint16_t slotNum){
   if (slotNum == 0) {
      return 1;
   } else {
      return 0;
   }
}

uint8_t cellUsageGet_getChannelOffset(uint16_t slotNum){
   return 0;
}

/*bool iDManager_isMyAddress(open_addr_t* addr){
return TRUE; //hack, hardcode addresses here if we want to use more than two motes
}*/

/*bool iDManager_getIsDAGroot(){
if (*(&eui64+3)==0x09)  // this is a GINA board (not a basestation)
return 0;
      else
return 1;
}*/