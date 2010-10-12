/*
 * The component which managing the buffer of packet
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#include "openwsn.h"
#include "openqueue.h"
#include "openserial.h"

//===================================== variables =============================

OpenQueueEntry_t queue[QUEUELENGTH];

//===================================== prototypes ============================
//===================================== public ================================

void openqueue_init() {
   uint8_t i;
   for (i=0;i<QUEUELENGTH;i++){
      openqueue_reset_entry(&(queue[i]));
   }
}

OpenQueueEntry_t* openqueue_getFreePacketBuffer() {
   uint8_t i;
   for (i=0;i<QUEUELENGTH;i++) {
      if (queue[i].owner==COMPONENT_NULL) {
         queue[i].owner=COMPONENT_OPENQUEUE;
         return &queue[i];
      }
   }
   return NULL;
}
error_t openqueue_freePacketBuffer(OpenQueueEntry_t* pkt) {
   uint8_t i;
   for (i=0;i<QUEUELENGTH;i++) {
      if (&queue[i]==pkt) {
         openqueue_reset_entry(&(queue[i]));
         return E_SUCCESS;
      }
   }
   return E_FAIL;
}

OpenQueueEntry_t* openqueue_inQueue(bool isADV) {
   uint8_t i;
   switch(isADV) {
      case IS_ADV:
         for (i=0;i<QUEUELENGTH;i++){
            if (queue[i].owner==COMPONENT_MAC && queue[i].creator==COMPONENT_RES) {
               return &queue[i];
            }
         }
         break;
      case IS_NOT_ADV:
         for (i=0;i<QUEUELENGTH;i++) {
            if (queue[i].owner==COMPONENT_MAC && queue[i].creator!=COMPONENT_RES) {
               return &queue[i];
            }
         }
         break;
   }
   return NULL;
}

error_t openqueue_removeAllPacketsToNeighbor(open_addr_t* neighbor) {
   error_t returnValue=E_FAIL;
   /*uint8_t i;
     for (i=0;i<QUEUELENGTH;i++){
     atomic if (queue[i].owner==COMPONENT_MAC && ((IEEE802154_ht*)(queue[i].payload))->dest==neighbor) {
     queue[i].owner=COMPONENT_NULL;
     queue[i].l2_retriesLeft=0;
     returnValue=E_SUCCESS;
     }
     }poipoistupid*/
   return returnValue;
}

void openqueue_removeAllOwnedBy(uint8_t owner) {
   uint8_t i;
   for (i=0;i<QUEUELENGTH;i++){
      if (queue[i].owner==owner) {
         openqueue_reset_entry(&(queue[i]));
      }
   }
}

void openqueue_reset_entry(OpenQueueEntry_t* entry) {
   //admin
   entry->creator                     = COMPONENT_NULL;
   entry->owner                       = COMPONENT_NULL;
   entry->payload                     = &(entry->packet[127]);
   entry->length                      = 0;
   //l4
   entry->l4_protocol                 = IANA_UNDEFINED;
   //l3
   entry->l3_destinationORsource.type = ADDR_NONE;
   //l2
   entry->l2_nextORpreviousHop.type   = ADDR_NONE;
   entry->l2_frameType                = IEEE154_TYPE_UNDEFINED;
   entry->l2_retriesLeft              = 0;
}

bool openqueue_debugPrint() {
   debugOpenQueueEntry_t output[QUEUELENGTH];
   uint8_t i;
   for (i=0;i<QUEUELENGTH;i++) {
      output[i].creator = queue[i].creator;
      output[i].owner   = queue[i].owner;
   }
   openserial_printStatus(STATUS_OPENQUEUEP_QUEUE,(uint8_t*)&output,QUEUELENGTH*sizeof(debugOpenQueueEntry_t));
   return TRUE;
}
