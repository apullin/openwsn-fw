/**
\brief Implementation of the IEEE802.15.4e RES layer

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
*/

#include "openwsn.h"
#include "res.h"
#include "idmanager.h"
#include "openserial.h"
#include "IEEE802154.h"
#include "IEEE802154E.h"
#include "openqueue.h"
#include "neighbors.h"
#include "timers.h"
#include "IEEE802154E.h"
#include "iphc.h"
#include "leds.h"
#include "packetfunctions.h"
#include "random.h"

//=========================== variables =======================================

typedef struct {
   uint16_t periodMaintenance;
   bool     busySending;
   uint8_t  dsn;                // current data sequence number
} res_vars_t;

res_vars_t res_vars;

//=========================== prototypes ======================================

error_t res_send_internal(OpenQueueEntry_t* msg);

//=========================== public ==========================================

void res_init() {
   res_vars.periodMaintenance = 16384+random_get16b()%32768; // fires after 1 sec on average
   res_vars.busySending       = FALSE;
   res_vars.dsn               = 0;
   timer_startPeriodic(TIMER_RES,res_vars.periodMaintenance);
}

bool res_debugPrint() {
   uint16_t output=0;
   output = neighbors_getMyDAGrank();
   openserial_printStatus(STATUS_RES_DAGRANK,(uint8_t*)&output,1);
   return TRUE;
}

//======= from upper layer

error_t res_send(OpenQueueEntry_t *msg) {
   msg->owner        = COMPONENT_RES;
   msg->l2_frameType = IEEE154_TYPE_DATA;
   return res_send_internal(msg);
}

//======= from lower layer

void task_resNotifSendDone() {
   OpenQueueEntry_t* msg;
   // get recently-sent packet from openqueue
   msg = openqueue_resGetSentPacket();
   if (msg==NULL) {
      // log the error
      openserial_printError(COMPONENT_RES,
                            ERR_NO_SENT_PACKET,
                            0,
                            0);
      // abort
      return;
   }
   msg->owner = COMPONENT_RES;
   if (msg->creator == COMPONENT_RES) {
      // discard (ADV) packets this component has created
      openqueue_freePacketBuffer(msg);
      // I can send the next ADV
      res_vars.busySending = FALSE;
      // restart a random timer
      res_vars.periodMaintenance = 16384+random_get16b()%32768;
      timer_startPeriodic(TIMER_RES,res_vars.periodMaintenance);
   } else {
      // send the rest up the stack
      iphc_sendDone(msg,msg->l2_sendDoneError);
   }
}

void task_resNotifReceive() {
   OpenQueueEntry_t* msg;
   // get received packet from openqueue
   msg = openqueue_resGetReceivedPacket();
   if (msg==NULL) {
      // log the error
      openserial_printError(COMPONENT_RES,
                            ERR_NO_RECEIVED_PACKET,
                            0,
                            0);
      // abort
      return;
   }
   msg->owner = COMPONENT_RES;
   // send the rest up the stack
   switch (msg->l2_frameType) {
      case IEEE154_TYPE_DATA:
         iphc_receive(msg);
         break;
      default:
         openqueue_freePacketBuffer(msg);
         openserial_printError(COMPONENT_RES,
                               ERR_MSG_UNKNOWN_TYPE,
                               msg->l2_frameType,
                               0);
         break;
   }
}

//======= timer

void timer_res_fired() {
   OpenQueueEntry_t* adv;
   
   // only send a packet if I received a sendDone for the previous.
   // the packet might be stuck in the queue for a long time for
   // example while the mote is synchronizing
   if (res_vars.busySending==FALSE) {
      // get a free packet buffer
      adv = openqueue_getFreePacketBuffer();
      if (adv==NULL) {
         openserial_printError(ERR_NO_FREE_PACKET_BUFFER,
                               COMPONENT_RES,
                               0,
                               0);
         return;
      }
      
      // declare ownership over that packet
      adv->creator = COMPONENT_RES;
      adv->owner   = COMPONENT_RES;
      
      // add ADV-specific header
      packetfunctions_reserveHeaderSize(adv,sizeof(IEEE802154E_ADV_ht));
      // the actual value of the current ASN will be written by the
      // IEEE802.15.4e when transmitting
      ((IEEE802154E_ADV_ht*)(adv->payload))->asn[0] = 0x00;
      ((IEEE802154E_ADV_ht*)(adv->payload))->asn[1] = 0x00;
      
      // some l2 information about this packet
      adv->l2_frameType = IEEE154_TYPE_BEACON;
      adv->l2_nextORpreviousHop.type = ADDR_16B;
      adv->l2_nextORpreviousHop.addr_16b[0] = 0xff;
      adv->l2_nextORpreviousHop.addr_16b[1] = 0xff;
      
      // put in queue for MAC to handle
      res_send_internal(adv);
      res_vars.busySending = TRUE;
   }
}

//=========================== private =========================================

/**
\brief Transfer packet to MAC.

This function adds a IEEE802.15.4 header to the packet and leaves it the 
OpenQueue buffer. The very last thing it does is assigning this packet to the 
virtual component COMPONENT_RES_TO_IEEE802154E. Whenever it gets a change,
IEEE802154E will handle the packet.

\param [in] msg The packet to the transmitted

\returns E_SUCCESS iff successful.
*/
error_t res_send_internal(OpenQueueEntry_t* msg) {
   // assign a number of retries
   if (packetfunctions_isBroadcastMulticast(&(msg->l2_nextORpreviousHop))==TRUE) {
      msg->l2_retriesLeft = 1;
   } else {
      msg->l2_retriesLeft = TXRETRIES;
   }
   // assign a TX power
   msg->l1_txPower = TX_POWER;
   // record the location, in the packet, where the l2 payload starts
   msg->l2_payload = msg->payload;
   // add a IEEE802.15.4 header
   ieee802154_prependHeader(msg,
                            msg->l2_frameType,
                            IEEE154_SEC_NO_SECURITY,
                            res_vars.dsn++,
                            &(msg->l2_nextORpreviousHop)
                            );
   // reserve space for 2-byte CRC
   packetfunctions_reserveFooterSize(msg,2);
   // change owner
   msg->owner  = COMPONENT_RES_TO_IEEE802154E;
   return E_SUCCESS;
}