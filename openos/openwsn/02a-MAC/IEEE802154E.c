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



//===================================== variables =============================

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
   OpenQueueEntry_t*  sendDoneMessage;
   error_t            sendDoneError;

//===================================== prototypes ============================

#include "IEEE802154_common.c"
   void       change_state(uint8_t newstate);
   void       resynchronize(bool resyncType, open_addr_t* node_id, timervalue_t dataGlobalSlotOffset, int16_t timeCorrection);
   void       slot_alarm_fired();
   void       fast_alarm_fired();
   void       endSlot();
   void       taskDebugPrint();             //task
   bool       mac_debugPrint();
   void       taskResetLosingLostTimers();  //task
   uint16_t   fastAlarm_getNow();
   uint16_t   slotAlarm_getNow();
   void fastAlarm_start(uint16_t duration);
   void slotAlarm_startAt(uint16_t startingPoint,uint16_t duration); 
   void fastAlarm_startAt(uint16_t startingPoint,uint16_t duration);
   //the two following tasks are used to break the asynchronicity: everything in MAC and below is async, all the above not
   void taskReceive(); //task
   void postTaskSendDone(OpenQueueEntry_t* param_sendDoneMessage, error_t param_sendDoneError);
   void taskSendDone(); //task

   
   //the following are prototypes for cellUsage -- which needs to be moved to a separate library
   uint8_t cellUsageGet_getType(uint16_t slotNum);
   uint8_t cellUsageGet_isTX(uint16_t slotNum);
   uint8_t cellUsageGet_isRX(uint16_t slotNum);
   uint8_t cellUsageGet_isADV(uint16_t slotNum);
   uint8_t cellUsageGet_getChannelOffset(uint16_t slotNum);
   bool iDManager_isMyAddress(open_addr_t* addr);
   bool iDManager_getIsDAGroot();
//===================================== public from upper layer ===============


void mac_init(){
    isSync = 0; 
    change_state(S_SYNCHRONIZING);
    dataFrameToSend = NULL;
    asn = 0;
    timer_startPeriodic(TIMER_MAC_PERIODIC,PERIODICTIMERPERIOD);
}


//a packet sent from the upper layer is simply stored into the OpenQueue buffer.
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

//wrapper function
void timer_mac_periodic_fired() {
       slot_alarm_fired(); //see next
}


void slot_alarm_fired(){
  
      asn_t                   temp_asn;
      uint8_t                 temp_state;
      bool                    temp_isSync;
      OpenQueueEntry_t*       temp_dataFrameToSend;
      //error_t                 temp_error;
      //open_addr_t             temp_addr_16b;
      //open_addr_t             temp_addr_64b;
      ieee802154_header_iht   transmitted_ieee154_header;
      uint8_t                 temp_channelOffset;

      
      /*This is an exmaple for a 10ms timeslot
      After the slot fires, we also start a backoff timer
      if we are a transmitter, we haev already loaded a packet into the radio buffer, and we simply tranmitt when the timer fires (say 2ms)
      as a receiver, we want to extend the timer (by 1ms, for exmaple) so we can listen to an incoming packet,
      which will hopefully have been sent at 2ms
      */
      timer_startOneShot(TIMER_MAC_BACKOFF,MINBACKOFF);
      //fastAlarmStartSlotTimestamp     = fastAlarm_getNow();
      //slotAlarmStartSlotTimestamp     = slotAlarm_getNow();
      
      asn++;

      openserial_stop();
   
      //reset WDT
      //atomic WDTCTL = WDTPW + WDTCNTCL + WDTSSEL; poipoi

      dataFrameToSend   = NULL;

      temp_asn             = asn;
      temp_state           = state;
      temp_isSync          = isSync;
      temp_dataFrameToSend = dataFrameToSend;

      //----- switch to/from S_SYNCHRONIZING
      if ((idmanager_getIsDAGroot())==TRUE) {
         //if I'm DAGroot, I'm synchronized
         taskResetLosingLostTimers();
         isSync=TRUE;
         if (state==S_SYNCHRONIZING) {//happens right after node becomes LBR
            endSlot();
            return;
         }
      } else if (temp_isSync==FALSE) {
         //If I'm not in sync, enter/stay in S_SYNCHRONIZING
         /*atomic if (temp_asn%2==1) {
            openserial_startOutput();
         } else {
            openserial_startInput();
         }*///poipoi
         if (temp_state!=S_SYNCHRONIZING) {
            change_state(S_SYNCHRONIZING);
            radio_rxOn(11);
            endSlot();
         }
         return;
      }

      //----- state error
      if (temp_state!=S_SLEEP) {     
         openserial_printError(COMPONENT_MAC,ERR_WRONG_STATE_IN_STARTSLOTTASK,
               (errorparameter_t)temp_state,(errorparameter_t)temp_asn%LENGTHCELLFRAME);
         endSlot(); //poipoi write
         return;
      }

      switch (cellUsageGet_getType(temp_asn%LENGTHCELLFRAME)) {
         case CELLTYPE_TXRX:
            //get a packet out of the buffer (if any)
           if (cellUsageGet_isTX(temp_asn%LENGTHCELLFRAME)){//poipoi || cellUsageGet_isSH_TX(temp_asn%LENGTHCELLFRAME)) {
                  dataFrameToSend = openqueue_inQueue(cellUsageGet_isADV(temp_asn%LENGTHCELLFRAME));
                  temp_dataFrameToSend = dataFrameToSend;                 
            }

            temp_channelOffset = cellUsageGet_getChannelOffset(temp_asn%LENGTHCELLFRAME);//poipoi write
            if (HOPPING_ENABLED) {
               frequencyChannel = ((temp_asn+temp_channelOffset)%16)+11;
            } else {
               frequencyChannel =((temp_channelOffset)%16)+11;
            }

            if (temp_dataFrameToSend!=NULL) {                                  //start the TX sequence
               dataFrameToSend->owner = COMPONENT_MAC;
               dataFrameToSend->l1_channel = frequencyChannel;
               transmitted_ieee154_header = retrieveIEEE802154header(temp_dataFrameToSend);
               if (cellUsageGet_isADV(temp_asn%LENGTHCELLFRAME)) {
                  //I will be sending an ADV frame
                  ((IEEE802154E_ADV_t*)((dataFrameToSend->payload)+transmitted_ieee154_header.headerLength))->timingInformation=temp_asn;// (globalTime.getASN());//poipoi implement
               }
               change_state(S_TX_TXDATAPREPARE);
               if (radio_prepare_send(dataFrameToSend)!=E_SUCCESS) {
                  //retry sending the packet later
                  temp_dataFrameToSend->l2_retriesLeft--;
                  if (temp_dataFrameToSend->l2_retriesLeft==0) {
                     postTaskSendDone(temp_dataFrameToSend,FAIL);
                  }
                  openserial_printError(COMPONENT_MAC,ERR_PREPARESEND_FAILED,
                        (errorparameter_t)temp_asn%LENGTHCELLFRAME,(errorparameter_t)0);
                  endSlot();
               };
               //fastAlarm_startAt(fastAlarmStartSlotTimestamp,TsTxOffset);//watchdog timer
            } else {
               if (cellUsageGet_isRX(temp_asn%LENGTHCELLFRAME)) {        //start the RX sequence
                  change_state(S_RX_RXDATAPREPARE);
                  radio_rxOn(frequencyChannel);
                  /*if (radio_rxOn(frequencyChannel)!=E_SUCCES) {
                     //abort
                     openserial_printError(COMPONENT_MAC,ERR_PREPARERECEIVE_FAILED,
                           (errorparameter_t)temp_asn%LENGTHCELLFRAME,(errorparameter_t)0);
                     endSlot();
                  };*/
                  
                  //add one milisecond to extend wait period for incoming packet
                  TBCCR1   = TBCCR1+21;   //add one extra ms to wait for incominf packet
                  TBCCTL1  = CCIE;        //enable interup
                  
                  //fastAlarm_startAt(fastAlarmStartSlotTimestamp,TsRxOffset);
               } else {                                                        //nothing to do, abort
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
                  cellUsageGet_getType(temp_asn%LENGTHCELLFRAME),0);
            endSlot();
            return;
      }
  
}

void radio_prepare_send_done(){//poipoi implememnt in phys layer
      asn_t   temp_asn;
      uint8_t temp_state;
      
      temp_asn   = asn;
      temp_state = state;
      
      switch (temp_state) {
         case S_TX_TXDATAPREPARE:
 
            break;
         case S_RX_TXACKPREPARE:

               change_state(S_RX_TXACKREADY);

            break;
         default:
            openserial_printError(COMPONENT_MAC,ERR_WRONG_STATE_IN_PREPARESENDDONE,
                  (errorparameter_t)temp_state,(errorparameter_t)temp_asn%LENGTHCELLFRAME);
            endSlot();
            break;
      }
}

void radioControl_prepareReceiveDone(error_t error){
      asn_t   temp_asn;
      uint8_t temp_state;
      
      temp_asn = asn;
      temp_state = state;
      
      if (error!=E_SUCCESS) {
         //abort
         openserial_printError(COMPONENT_MAC,ERR_PREPARERECEIVEDONE_FAILED,
               (errorparameter_t)error,(errorparameter_t)temp_asn%LENGTHCELLFRAME);
         endSlot();
      }
      switch (temp_state) {
         case S_TX_RXACKPREPARE:
            change_state(S_TX_RXACKREADY);
            break;
         case S_RX_RXDATAPREPARE:
            change_state(S_RX_RXDATAREADY);
            break;
         case S_SYNCHRONIZING:
            //if ( (RadioControl.receiveNow(TIME_LIMITED_RX,TsRxWaitTime))!=SUCCESS ) { poipoi make so radio only stays on for some time
           radio_rxOn(frequencyChannel);   
           /*if(radio_rxOn(frequencyChannel)!=E_SUCCESS){
               //abort
               openserial_printError(COMPONENT_MAC,ERR_RECEIVENOW_FAILED,
                     (errorparameter_t)temp_state,(errorparameter_t)temp_asn%LENGTHCELLFRAME);
               endSlot();
            }poipoi*/
            break;
         default:
            openserial_printError(COMPONENT_MAC,ERR_WRONG_STATE_IN_PREPARERECEIVEDONE,
                  (errorparameter_t)temp_state,(errorparameter_t)temp_asn%LENGTHCELLFRAME);
            endSlot();
            break;
      }
}

//wrapper
void timer_mac_backoff_fired(){
  fast_alarm_fired();//see next
}

void fast_alarm_fired() {
      asn_t             temp_asn;
      uint8_t           temp_state;
      OpenQueueEntry_t* temp_dataFrameToSend;
      
      temp_asn             = asn;
      temp_state           = state;
      temp_dataFrameToSend = dataFrameToSend;

      switch (temp_state) {
         /*------------------- TX sequence ------------------------*/
         case S_TX_TXDATAPREPARE:                                    //[timer fired] transmitter (ERROR state)
            //I'm a transmitter, didn't have time to prepare for TX
            postTaskSendDone(temp_dataFrameToSend,FAIL);
            openserial_printError(COMPONENT_MAC,ERR_NO_TIME_TO_PREPARE_TX,
                  (errorparameter_t)temp_asn%LENGTHCELLFRAME,0);
            endSlot();
            break;
         case S_TX_TXDATAREADY:                                      //[timer fired] transmitter
            //I'm a transmitter, Tx data now
            change_state(S_TX_TXDATA);
            if ((radio_send_now())!=E_SUCCESS) {
               //retry later
               temp_dataFrameToSend->l2_retriesLeft--;
               if (temp_dataFrameToSend->l2_retriesLeft==0) {
                  postTaskSendDone(temp_dataFrameToSend,FAIL);
               }
               openserial_printError(COMPONENT_MAC,ERR_SENDNOW_FAILED,
                     (errorparameter_t)temp_state,(errorparameter_t)temp_asn%LENGTHCELLFRAME);
               endSlot();
            }
            break;
         case S_TX_RXACKREADY:                                       //[timer fired] transmitter
            //I'm a transmitter, switch on RX for ACK now
            //this is done automatically after Tx is finished, rx is already on.
            //I'm calling receiveNow anyways because it reports receivedNothing
            change_state(S_TX_RXACK);
            radio_rxOn(frequencyChannel);
           /* if ( radio_rxOn(frequencyChannel)!=E_SUCCESS ) {
               //abort
               openserial_printError(COMPONENT_MAC,ERR_RECEIVENOW_FAILED,
                     (errorparameter_t)temp_state,(errorparameter_t)0);
               endSlot();
            };poipoi*/
            break;//from here***

            /*------------------- RX sequence -----------------------*/
         case S_RX_RXDATAPREPARE:                                    //[timer fired] receiver (ERROR state)
            //I'm a receiver, didn't have time to prepare for RX
            openserial_printError(COMPONENT_MAC,ERR_NO_TIME_TO_PREPARE_RX,
                  (errorparameter_t)temp_asn%LENGTHCELLFRAME,0);
            endSlot();
            break;
         case S_RX_RXDATAREADY:                                      //[timer fired] receiver
            //I'm a receiver, switch RX radio on for data now
            change_state(S_RX_RXDATA);
            
            radio_rxOn(frequencyChannel);
            
            /*if ( (call RadioControl.receiveNow(TIME_LIMITED_RX,TsRxWaitTime))!=SUCCESS ) {
               //abort
               openserial_printError(COMPONENT_MAC,ERR_RECEIVENOW_FAILED,
                     (errorparameter_t)temp_state,(errorparameter_t)0);
               endSlot();
            };poipoi*/
            break;
         case S_RX_TXACKPREPARE:                                     //[timer fired] receiver (ERROR state)
            //I'm a receiver, didn't have time to prepare ACK
            openserial_printError(COMPONENT_MAC,ERR_NO_TIME_TO_PREPARE_ACK,
                  (errorparameter_t)temp_asn%LENGTHCELLFRAME,0);
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
                     (errorparameter_t)temp_state,(errorparameter_t)temp_asn%LENGTHCELLFRAME);
               endSlot();
            }*/
            break;

         default:
            openserial_printError(COMPONENT_MAC,ERR_WRONG_STATE_IN_FASTTIMER_FIRED,
                  (errorparameter_t)temp_state,(errorparameter_t)temp_asn%LENGTHCELLFRAME);
            endSlot();
            break;
      }
 }


//call in mac, not from radio, if no interupt by now, turn radio off
void radioControl_receivedNothing(error_t error){
 
      asn_t                   temp_asn;
      uint8_t                 temp_state;
      OpenQueueEntry_t*       temp_dataFrameToSend;

      temp_asn             = asn;
      temp_state           = state;
      temp_dataFrameToSend = dataFrameToSend;

      switch(temp_state) {
         case S_RX_RXDATA:                                           //[receivedNothing] receiver (WARNING state)
            //I'm a receiver, didn't receive data
            endSlot();
            break;
         case S_TX_RXACK:                                            //[receivedNothing] transmitter (WARNING state)
            //I'm a transmitter, didn't receive ACK (end of TX sequence)
            
           /// major poipoi, implement both of these later for true tsch neighbor management
           //cellStats_indicateUse(temp_asn%LENGTHCELLFRAME,WAS_NOT_ACKED);
           //count num of sent packets
           //neighborStats_indicateTx(&(temp_dataFrameToSend->l2_nextORpreviousHop),WAS_NOT_ACKED);
           
           temp_dataFrameToSend->l2_retriesLeft--;
            if (temp_dataFrameToSend->l2_retriesLeft==0) {
               postTaskSendDone(temp_dataFrameToSend,FAIL);
            }
            endSlot();
            break;
         case S_SYNCHRONIZING:                                       //[receivedNothing] synchronizer
            //it's OK not to receive anything after TsRxWaitTime when trying to synchronize
            break;
         default:
            openserial_printError(COMPONENT_MAC,ERR_WRONG_STATE_IN_RECEIVEDNOTHING,
                  (errorparameter_t)temp_state,(errorparameter_t)temp_asn%LENGTHCELLFRAME);
            endSlot();
            break;
      }
      
}

void radio_send_now_done(error_t error){//poipoi call from phy layer
      asn_t                   temp_asn;
      uint8_t                 temp_state;
      OpenQueueEntry_t*       temp_dataFrameToSend;

      temp_asn             = asn;
      temp_state           = state;
      temp_dataFrameToSend = dataFrameToSend;
         
      switch (temp_state) {
         case S_TX_TXDATA:                                           //[sendNowDone] transmitter
            //I'm a transmitter, finished sending data
            if (error!=E_SUCCESS) {
               //retry later
               temp_dataFrameToSend->l2_retriesLeft--;
               if (temp_dataFrameToSend->l2_retriesLeft==0) {
                  postTaskSendDone(temp_dataFrameToSend,FAIL);
               }
               openserial_printError(COMPONENT_MAC,ERR_SENDNOWDONE_FAILED,
                     (errorparameter_t)temp_state,
                     (errorparameter_t)temp_asn%LENGTHCELLFRAME);
               endSlot();
               return;
            }
            if (cellUsageGet_isADV(temp_asn%LENGTHCELLFRAME)==TRUE) {
               //ADV slot, don't have to listen for ACK
               /// major poipoi, implement all three of these later for true tsch neighbor management
               //call NeighborStats.indicateTx(&(temp_dataFrameToSend->l2_nextORpreviousHop),WAS_NOT_ACKED);
               //call CellStats.indicateUse(temp_asn%LENGTHCELLFRAME,WAS_NOT_ACKED);
               //postTaskSendDone(temp_dataFrameToSend,SUCCESS);
               
              endSlot();
            } else {
               fastAlarm_start(TsRxAckDelay);
               change_state(S_TX_RXACKREADY);
            }
            break;
         case S_RX_TXACK:                                            //[sendNowDone] receiver
            //I'm a receiver, finished sending ACK (end of RX sequence)
            if (error!=E_SUCCESS) {
               //don't do anything if error==FAIL
               openserial_printError(COMPONENT_MAC,ERR_SENDNOWDONE_FAILED,
                     (errorparameter_t)temp_state,
                     (errorparameter_t)temp_asn%LENGTHCELLFRAME);
            }
            /* //sync off of DATA I received before I sent ACK
             * poipoi for simplicity, only resync from ADV
             resynchronize(FRAME_BASED_RESYNC,
             &(frameReceived->l2_nextORpreviousHop),
             frameReceived->l1_rxTimestamp,
             (int16_t)((int32_t)(frameReceived->l1_rxTimestamp)-(int32_t)radio_delay)-(int32_t)TsTxOffset);*/
            
            taskReceive(); 
            endSlot();
            break;
         default:
            openserial_printError(COMPONENT_MAC,ERR_WRONG_STATE_IN_SUBSEND_SENDDONE,
                  (errorparameter_t)temp_state,(errorparameter_t)temp_asn%LENGTHCELLFRAME);
            endSlot();
            break;
      }
}

void postTaskSendDone(OpenQueueEntry_t* param_sendDoneMessage, error_t param_sendDoneError) {
      
  //just call senddone
  if (sendDoneMessage!=NULL) {
         openserial_printError(COMPONENT_MAC,ERR_BUSY_SENDDONE,
               (errorparameter_t)state,(errorparameter_t)asn%LENGTHCELLFRAME);
      }
      sendDoneMessage = param_sendDoneMessage;
      sendDoneError   = param_sendDoneError;
      
      taskSendDone(); //poipoi wthat?
}

void taskSendDone() {
      OpenQueueEntry_t*  temp_sendDoneMessage;
      error_t            temp_sendDoneError;
      
      temp_sendDoneMessage = sendDoneMessage;
      temp_sendDoneError   = sendDoneError;
      
      //openSendFromUpper_sendDone(temp_sendDoneMessage,temp_sendDoneError);
      nores_sendDone(temp_sendDoneMessage,temp_sendDoneError);
      sendDoneMessage = NULL;
}

void endSlot() {
      //asn_t   temp_asn;
      //uint8_t temp_state;
      
     // temp_asn   = asn;
     // temp_state = state;
      if (packetACK!=NULL) {
            //malloc_freePacketBuffer(packetACK);
            openqueue_freePacketBuffer(packetACK);
            packetACK=NULL;
      }
      
      radio_rfOff();
      /*if (radio_rfOff()!=E_SUCCESS) {
         //abort
         openserial_printError(COMPONENT_MAC,ERR_RFOFF_FAILED,
               (errorparameter_t)temp_state,(errorparameter_t)temp_asn%LENGTHCELLFRAME);
      }*/
      change_state(S_SLEEP);
}

//===================================== public from lower layer ===============

void mac_sendDone(OpenQueueEntry_t* pkt, error_t error) {
    taskSendDone();
}
//===================================== private ===============================

//*******************************from here***********************/
void radio_packet_received(OpenQueueEntry_t* msg) {
      asn_t                   temp_asn;
      uint8_t                 temp_state;
      //bool                    temp_isSync;
      OpenQueueEntry_t*       temp_dataFrameToSend;
      OpenQueueEntry_t*       temp_packetACK;
      error_t                 temp_error;
      ieee802154_header_iht   received_ieee154_header;
      ieee802154_header_iht   transmitted_ieee154_header;

      temp_asn             = asn;
     // temp_isSync          = isSync;
      temp_state           = state;
      temp_dataFrameToSend = dataFrameToSend;
 

      msg->owner = COMPONENT_MAC;

      received_ieee154_header = retrieveIEEE802154header(msg);
      packetfunctions_tossHeader(msg,received_ieee154_header.headerLength);
      packetfunctions_tossFooter(msg,2);

      msg->l2_frameType = received_ieee154_header.frameType;
      memcpy(&(msg->l2_nextORpreviousHop),&(received_ieee154_header.src),sizeof(open_addr_t));


      if (received_ieee154_header.frameType==IEEE154_TYPE_DATA &&
            !(iDManager_isMyAddress(&received_ieee154_header.panid))) {
          openserial_printError(COMPONENT_MAC,ERR_WRONG_PANID,
               (errorparameter_t)received_ieee154_header.panid.panid[0]*256+received_ieee154_header.panid.panid[1],
               (errorparameter_t)0);
        openqueue_freePacketBuffer(msg);
         return;
      }

      switch (temp_state) {

         /*------------------- TX sequence ------------------------*/
         case S_TX_RXACK:                                            //[receive] transmitter
            //I'm a transmitter, just received ACK (end of TX sequence)
            transmitted_ieee154_header = retrieveIEEE802154header(temp_dataFrameToSend);
            if (received_ieee154_header.dsn == transmitted_ieee154_header.dsn) {
               //I'm a transmitter, sync off of ACK message
               /* poipoi for simplicity, only resync from ADV
                  resynchronize(ACK_BASED_RESYNC,
                  &received_ieee154_header.src,
                  TsTxOffset,
                  (((IEEE802154E_ACK_ht*)(msg->payload))->timeCorrection));//poipoi /32  poipoipoipoi*/
               
               //poipoi implement
               //cellStats_indicateUse(temp_asn%LENGTHCELLFRAME,WAS_ACKED);
               //neighborStats_indicateTx(&(temp_dataFrameToSend->l2_nextORpreviousHop),WAS_ACKED);
              
               //postTaskSendDone(temp_dataFrameToSend,SUCCESS); poipoi what?
            } else {
              //implement these poipoi
             // cellStats.indicateUse(temp_asn%LENGTHCELLFRAME,WAS_NOT_ACKED);
             //  neighborStats.indicateTx(&(temp_dataFrameToSend->l2_nextORpreviousHop),WAS_NOT_ACKED);
               temp_dataFrameToSend->l2_retriesLeft--;
               if (temp_dataFrameToSend->l2_retriesLeft==0) {
                  postTaskSendDone(temp_dataFrameToSend,FAIL);
               }
            }
            openqueue_freePacketBuffer(msg);//free ACK
            endSlot();
            break;

            /*------------------- RX sequence ------------------------*/
         case S_SYNCHRONIZING:
         case S_RX_RXDATA:                                           //[receive] receiver
            //I'm a receiver, just received data
            if (iDManager_isMyAddress(&(received_ieee154_header.dest)) && received_ieee154_header.ackRequested) {
               //ACK requested
              radio_rfOff();
              /*if (radio_rfOff()!=E_SUCCESS) {
                  //do nothing about it
                  openserial_printError(COMPONENT_MAC,ERR_RFOFF_FAILED,
                        (errorparameter_t)temp_state,(errorparameter_t)temp_asn%LENGTHCELLFRAME);
               }*/
               fastAlarm_start(TsTxAckDelay);
               change_state(S_RX_TXACKPREPARE);
               
               packetACK = openqueue_getFreePacketBuffer();
               temp_packetACK = packetACK;
               
               if (temp_packetACK==NULL) {
                  openserial_printError(COMPONENT_MAC,ERR_NO_FREE_PACKET_BUFFER,
                        (errorparameter_t)0,(errorparameter_t)0);
                  openqueue_freePacketBuffer(msg);
                  endSlot();
                  return;
               }
               temp_packetACK->creator       = COMPONENT_MAC;
               temp_packetACK->owner         = COMPONENT_MAC;
               //ACK payload
               packetfunctions_reserveHeaderSize(temp_packetACK,sizeof(IEEE802154E_ACK_ht));
               ((IEEE802154E_ACK_ht*)(temp_packetACK->payload))->dhrAckNack     = IEEE154E_ACK_dhrAckNack_DEFAULT;
               ((IEEE802154E_ACK_ht*)(temp_packetACK->payload))->timeCorrection =
                  (int16_t)((int32_t)(TsTxOffset+radio_delay)-(int32_t)(msg->l1_rxTimestamp));//poipoi *32
               //154 header
               prependIEEE802154header(temp_packetACK,
                     IEEE154_TYPE_ACK,
                     IEEE154_SEC_NO_SECURITY,
                     received_ieee154_header.dsn,
                     NULL
                     );
               //l2 metadata
               temp_packetACK->l2_retriesLeft  = 1;
               //l1_metadata
               temp_packetACK->l1_txPower        = TX_POWER;
               temp_packetACK->l1_channel = frequencyChannel;
               temp_error = radio_prepare_send(temp_packetACK);
               if (temp_error!=E_SUCCESS) {
                  //abort
                  openserial_printError(COMPONENT_MAC,ERR_PREPARESEND_FAILED,
                        (errorparameter_t)temp_asn%LENGTHCELLFRAME,(errorparameter_t)0);
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
               taskReceive();
               endSlot();
            } else {
               openqueue_freePacketBuffer(msg);
               endSlot();
            }
            break;

         default:
            openqueue_freePacketBuffer(msg);
            endSlot();
            openserial_printError(COMPONENT_MAC,ERR_WRONG_STATE_IN_RECEIVE,
                  (errorparameter_t)temp_state,(errorparameter_t)temp_asn%LENGTHCELLFRAME);
            endSlot();
            break;
      }
}


 void taskReceive() {
      //asn_t              temp_asn;
      OpenQueueEntry_t*  temp_frameReceived;
      //temp_asn = asn;
      temp_frameReceived = frameReceived;

      
      if (temp_frameReceived->length>0) {
      //packet contains payload destined to an upper layer
          nores_receive(temp_frameReceived);
      } else {
        openqueue_freePacketBuffer(temp_frameReceived);
      }
      temp_frameReceived = NULL;
     
 }

   /*------------------------------ resynchronization ---------------------------------*/
   void resynchronize(bool resyncType, open_addr_t* node_id, timervalue_t dataGlobalSlotOffset, int16_t timeCorrection) {

      bool          temp_isSync;
     // open_addr_t   timeParent;
      bool          iShouldSynchronize;

 
      temp_isSync = isSync;
     

      if ((iDManager_getIsDAGroot())==FALSE) {        //I resync only if I'm not a DAGroot
         //---checking whether I should synchronize
         iShouldSynchronize=FALSE;
         if (temp_isSync==FALSE) {                    //I'm not synchronized, I sync off of all ADV packets
            if (resyncType==FRAME_BASED_RESYNC) {
               iShouldSynchronize=TRUE;
               isSync=TRUE;
               openserial_printError(COMPONENT_MAC,ERR_ACQUIRED_SYNC,0,0);//not an error!
               //call Leds.led2On(); poipoi
            }
         } else {                                //I'm already synchronized
           
           //implement poipoi
           /*neighborGet_getPreferredParent(&timeParent,node_id->type);
            if (timeParent.type!=ADDR_NONE) {  //I have a timeparent, I sync off of any packet from it
               if (packetfunctions_sameAddress(&timeParent,node_id)) {
                  iShouldSynchronize=TRUE;
               }
            } else {                        //I don't have a timeparent, I sync off of any packet
               iShouldSynchronize=TRUE;
            }end implement*/
         }
         //---synchronize iif I need to
         if (iShouldSynchronize==TRUE) {
 
               if (resyncType==FRAME_BASED_RESYNC) {
                  if (dataGlobalSlotOffset!=INVALID_TIMESTAMP){
                     if (slotAlarmStartSlotTimestamp+dataGlobalSlotOffset<slotAlarm_getNow()) {
                        slotAlarm_startAt((uint32_t)((int32_t)slotAlarmStartSlotTimestamp+(int32_t)timeCorrection),SLOT_TIME);
                     } else {
                         isSync=FALSE;
                         openserial_printError(COMPONENT_MAC,ERR_SYNC_RACE_CONDITION,
                              (errorparameter_t)asn%LENGTHCELLFRAME,
                              (errorparameter_t)dataGlobalSlotOffset);
                        //call Leds.led2Off(); poipoi
                        slotAlarm_startAt((slotAlarm_getNow())-(SLOT_TIME/2),SLOT_TIME);
                        endSlot();
                     }
                  }
               } else {
                  slotAlarm_startAt((uint32_t)((int32_t)slotAlarmStartSlotTimestamp+(int32_t)timeCorrection),SLOT_TIME);
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

void fastAlarm_start(uint16_t duration) {
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
}


bool mac_debugPrint() {
   return FALSE;
}

//legacy
void timer_mac_watchdog_fired(){
  
}

//*********************************hardcore hack -- move later **********//
//*********************************cellUsage functions*******************//

uint8_t cellUsageGet_getType(uint16_t slotNum){
  if(slotNum < 3)
     return CELLTYPE_TXRX;
  else if(slotNum == 4)
      return CELLTYPE_RXSERIAL;
  else
      return CELLTYPE_OFF;
}


uint8_t cellUsageGet_isTX(uint16_t slotNum){
  if(slotNum == 0)
    return 0;
  else if(slotNum == 1){
    if (*(&eui64+3)==0x09)  // this is a GINA board (not a basestation)
      return 0;
    else 
      return 1;
  }else if(slotNum == 2){
    if (*(&eui64+3)==0x09)  // this is a GINA board (not a basestation)
      return 1;
    else 
      return 0;
  }
  else
    return 0; 
}



uint8_t cellUsageGet_isRX(uint16_t slotNum){
  if(slotNum == 0){
    return 0;
  }
  else if(slotNum == 1){
    if (*(&eui64+3)==0x09)  // this is a GINA board (not a basestation)
      return 1;
    else 
      return 0;
  }else if(slotNum == 2){
    if (*(&eui64+3)==0x09)  // this is a GINA board (not a basestation)
      return 0;
    else 
      return 1;
  }
  else
    return 0; 
}

uint8_t cellUsageGet_isADV(uint16_t slotNum){
  if(slotNum == 0)
    return 1;
  else
    return 0;
}

uint8_t cellUsageGet_getChannelOffset(uint16_t slotNum){
  return 0;
}

bool iDManager_isMyAddress(open_addr_t* addr){
  return TRUE; //hack, hardcode addresses here if we want to use more than two motes
}

bool iDManager_getIsDAGroot(){
      if (*(&eui64+3)==0x09)  // this is a GINA board (not a basestation)
        return 0;
      else
        return 1;
}

                               