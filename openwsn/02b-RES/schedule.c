/*
 * Manages the TSCH schedule
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
 */

#include "OpenWSN.h"
#include "schedule.h"
#include "openserial.h"

//===================================== variables ==============================

cellUsageInformation_t cellTable[SCHEDULELENGTH];
slotOffset_t           debugPrintSlotOffset;

//===================================== prototypes =============================

void schedule_init() {
   uint8_t slotCounter;
   //all slots OFF
   for (slotCounter=0;slotCounter<SCHEDULELENGTH;slotCounter++){
      cellTable[slotCounter].type          = CELLTYPE_OFF;
      cellTable[slotCounter].channelOffset = 0;
      cellTable[slotCounter].neighbor.type = ADDR_NONE;
      cellTable[slotCounter].numUsed       = 0;
      cellTable[slotCounter].numTxACK      = 0;
      cellTable[slotCounter].timestamp     = 0;
   }
   //slot 0 is advertisement slot
   cellTable[0].type                       = CELLTYPE_ADV;
   //slot 2 is receive over serial
   cellTable[2].type                       = CELLTYPE_RXSERIAL;
   
   // for debug print
   debugPrintSlotOffset                    = 0;
}

cellType_t schedule_getType(slotOffset_t slotOffset) {
   return cellTable[slotOffset].type;
}
channelOffset_t schedule_getChannelOffset(slotOffset_t slotOffset) {
   return cellTable[slotOffset].channelOffset;
}
open_addr_t schedule_getNeighbor(slotOffset_t slotOffset) {
   return cellTable[slotOffset].neighbor;
}


void schedule_indicateUse(asn_t asn, bool ack){
   uint16_t slotOffset;
   
   slotOffset = asn%SCHEDULELENGTH;
   if (cellTable[slotOffset].numUsed==0xFF) {
      cellTable[slotOffset].numUsed/=2;
      cellTable[slotOffset].numTxACK/=2;
   }
   cellTable[slotOffset].numUsed++;
   if (ack==TRUE) {
      cellTable[slotOffset].numTxACK++;
   }
   cellTable[slotOffset].timestamp=asn;
}

bool schedule_debugPrint() {
   debugCellUsageInformation_t temp;
   debugPrintSlotOffset = (debugPrintSlotOffset+1)%SCHEDULELENGTH;
   temp.row        = debugPrintSlotOffset;
   temp.cellUsage  = cellTable[debugPrintSlotOffset];
   openserial_printStatus(STATUS_SCHEDULE_CELLTABLE,
                          (uint8_t*)&temp,
                          sizeof(debugCellUsageInformation_t));
   return TRUE;
}