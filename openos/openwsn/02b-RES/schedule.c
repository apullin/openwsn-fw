/*
 * Manages the IEEE802.15.4e schedule
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
 */

#include "OpenWSN.h"
#include "schedule.h"
#include "openserial.h"

//===================================== variables =============================

cellUsageInformation_t cellTable[SCHEDULELENGTH];
slotOffset_t           debugPrintSlotOffset;

//===================================== prototypes ============================

//===================================== public ================================

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
   //slot 1 is receive slot
   cellTable[1].type                       = CELLTYPE_RX;
   //slot 2 is transmit slot to neighbor 0xffffffffffffffff
   cellTable[2].type                       = CELLTYPE_TX;
   cellTable[2].neighbor.type              = ADDR_64B;
   cellTable[2].neighbor.addr_64b[0]       = 0xff;
   cellTable[2].neighbor.addr_64b[1]       = 0xff;
   cellTable[2].neighbor.addr_64b[2]       = 0xff;
   cellTable[2].neighbor.addr_64b[3]       = 0xff;
   cellTable[2].neighbor.addr_64b[4]       = 0xff;
   cellTable[2].neighbor.addr_64b[5]       = 0xff;
   cellTable[2].neighbor.addr_64b[6]       = 0xff;
   cellTable[2].neighbor.addr_64b[7]       = 0xff;
   //slot 3 is transmit slot to neighbor 0x14159209022b0087
   cellTable[3].type                       = CELLTYPE_TX;
   cellTable[3].neighbor.type              = ADDR_64B;
   cellTable[3].neighbor.addr_64b[0]       = 0x14;
   cellTable[3].neighbor.addr_64b[1]       = 0x15;
   cellTable[3].neighbor.addr_64b[2]       = 0x92;
   cellTable[3].neighbor.addr_64b[3]       = 0x09;
   cellTable[3].neighbor.addr_64b[4]       = 0x02;
   cellTable[3].neighbor.addr_64b[5]       = 0x2b;
   cellTable[3].neighbor.addr_64b[6]       = 0x00;
   cellTable[3].neighbor.addr_64b[7]       = 0x87;
   
   // for debug print
   debugPrintSlotOffset                    = 0;
}

cellType_t schedule_getType(asn_t asn_param) {
   uint16_t slotOffset;
   slotOffset = asn_param%SCHEDULELENGTH;
   return cellTable[slotOffset].type;
}

channelOffset_t schedule_getChannelOffset(asn_t asn_param) {
   uint16_t slotOffset;
   slotOffset = asn_param%SCHEDULELENGTH;
   return cellTable[slotOffset].channelOffset;
}

void schedule_getNeighbor(asn_t asn_param, open_addr_t* addrToWrite) {
   uint16_t slotOffset;
   slotOffset = asn_param%SCHEDULELENGTH;
   memcpy(addrToWrite,&cellTable[slotOffset].neighbor,sizeof(open_addr_t));
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

//===================================== private ===============================