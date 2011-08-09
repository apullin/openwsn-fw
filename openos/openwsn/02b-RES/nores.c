/*
 * Implementation of noRES
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#include "openwsn.h"
#include "nores.h"
#include "idmanager.h"
#include "openserial.h"
#include "IEEE802154.h"
#include "IEEE802154E.h"
#include "openqueue.h"
#include "neighbors.h"
#include "IEEE802154E.h"
#include "iphc.h"
#include "packetfunctions.h"

//===================================== variables =============================

//===================================== prototypes ============================

//===================================== public ================================

void nores_init() {
}

//===================================== public with upper =====================

error_t nores_send(OpenQueueEntry_t *msg) {
   msg->owner = COMPONENT_RES;
   msg->l2_frameType = IEEE154_TYPE_DATA;
   return mac_send(msg);
}

//===================================== public with lower =====================

void nores_sendDone(OpenQueueEntry_t* msg, error_t error) {
   msg->owner = COMPONENT_RES;
   iphc_sendDone(msg,error);
}

void nores_receive(OpenQueueEntry_t* msg) {
   msg->owner = COMPONENT_RES;
   switch (msg->l2_frameType) {
      case IEEE154_TYPE_DATA:
         iphc_receive(msg);
         break;
      default:
         openqueue_freePacketBuffer(msg);
         openserial_printError(COMPONENT_RES,ERR_MSG_UNKNOWN_TYPE,msg->l2_frameType,0);
         break;
   }
}

bool nores_debugPrint() {
   uint16_t output=0;
   output = neighbors_getMyDAGrank();
   openserial_printStatus(STATUS_RES_DAGRANK,(uint8_t*)&output,1);
   return TRUE;
}
