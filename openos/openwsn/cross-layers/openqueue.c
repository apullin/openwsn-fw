/**
\brief The component which managing the buffer of packet

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/

#include "openwsn.h"
#include "openqueue.h"
#include "openserial.h"
#include "packetfunctions.h"

//=========================== variables =======================================

typedef struct {
   OpenQueueEntry_t queue[QUEUELENGTH];
} openqueue_vars_t;

openqueue_vars_t openqueue_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

void openqueue_init() {
   uint8_t i;
   for (i=0;i<QUEUELENGTH;i++){
      openqueue_reset_entry(&(openqueue_vars.queue[i]));
   }
}

OpenQueueEntry_t* openqueue_getFreePacketBuffer() {
   uint8_t i;
   for (i=0;i<QUEUELENGTH;i++) {
      if (openqueue_vars.queue[i].owner==COMPONENT_NULL) {
         openqueue_vars.queue[i].owner=COMPONENT_OPENQUEUE;
         return &openqueue_vars.queue[i];
      }
   }
   return NULL;
}
error_t openqueue_freePacketBuffer(OpenQueueEntry_t* pkt) {
   uint8_t i;
   for (i=0;i<QUEUELENGTH;i++) {
      if (&openqueue_vars.queue[i]==pkt) {
         openqueue_reset_entry(&(openqueue_vars.queue[i]));
         return E_SUCCESS;
      }
   }
   return E_FAIL;
}

OpenQueueEntry_t* openqueue_getDataPacket(open_addr_t* toNeighbor) {
   uint8_t i;
   for (i=0;i<QUEUELENGTH;i++) {
      if (openqueue_vars.queue[i].owner==COMPONENT_RES_TO_IEEE802154E &&
         packetfunctions_sameAddress(toNeighbor,&openqueue_vars.queue[i].l2_nextORpreviousHop)) {
         return &openqueue_vars.queue[i];
      }
   }
   return NULL;
}

OpenQueueEntry_t* openqueue_getAdvPacket() {
   uint8_t i;
   for (i=0;i<QUEUELENGTH;i++) {
      if (openqueue_vars.queue[i].owner==COMPONENT_RES_TO_IEEE802154E &&
          openqueue_vars.queue[i].creator==COMPONENT_RES) {
         return &openqueue_vars.queue[i];
      }
   }
   return NULL;
}

error_t openqueue_removeAllPacketsToNeighbor(open_addr_t* neighbor) {
   error_t returnValue=E_FAIL;
   /*uint8_t i;
     for (i=0;i<QUEUELENGTH;i++){
     atomic if (openqueue_vars.queue[i].owner==COMPONENT_MAC && ((IEEE802154_ht*)(openqueue_vars.queue[i].payload))->dest==neighbor) {
     openqueue_vars.queue[i].owner=COMPONENT_NULL;
     openqueue_vars.queue[i].l2_retriesLeft=0;
     returnValue=E_SUCCESS;
     }
     }poipoistupid*/
   return returnValue;
}

void openqueue_removeAllOwnedBy(uint8_t owner) {
   uint8_t i;
   for (i=0;i<QUEUELENGTH;i++){
      if (openqueue_vars.queue[i].owner==owner) {
         openqueue_reset_entry(&(openqueue_vars.queue[i]));
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
      output[i].creator = openqueue_vars.queue[i].creator;
      output[i].owner   = openqueue_vars.queue[i].owner;
   }
   openserial_printStatus(STATUS_OPENQUEUE_QUEUE,(uint8_t*)&output,QUEUELENGTH*sizeof(debugOpenQueueEntry_t));
   return TRUE;
}

//=========================== private =========================================