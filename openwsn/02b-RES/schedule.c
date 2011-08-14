/**
\brief Manages the IEEE802.15.4e schedule

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
*/

#include "OpenWSN.h"
#include "schedule.h"
#include "openserial.h"
#include "idmanager.h"

//=========================== variables =======================================

typedef struct {
   cellUsageInformation_t cellTable[SCHEDULELENGTH];
   slotOffset_t           debugPrintRow;
} schedule_vars_t;

schedule_vars_t schedule_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

void schedule_init() {
   uint8_t slotCounter;
   // for debug print
   debugPrintSlotOffset                    = 0;
   //all slots OFF
   for (slotCounter=0;slotCounter<SCHEDULELENGTH;slotCounter++){
      schedule_vars.cellTable[slotCounter].type          = CELLTYPE_OFF;
      schedule_vars.cellTable[slotCounter].channelOffset = 0;
      schedule_vars.cellTable[slotCounter].neighbor.type = ADDR_NONE;
      schedule_vars.cellTable[slotCounter].numUsed       = 0;
      schedule_vars.cellTable[slotCounter].numTxACK      = 0;
      schedule_vars.cellTable[slotCounter].timestamp     = 0;
   }
   //slot 0 is advertisement slot
   schedule_vars.cellTable[0].type                       = CELLTYPE_ADV;
   //slot 1 TX@MASTER, RX@SLAVE
   if (idmanager_getMyID(ADDR_16B)->addr_16b[1]==DEBUG_MOTEID_MASTER) {
      cellTable[1].type                    = CELLTYPE_TX;
   } else {
      cellTable[1].type                    = CELLTYPE_RX;
   }
   cellTable[1].neighbor.type              = ADDR_64B;
   cellTable[1].neighbor.addr_64b[0]       = 0x14;
   cellTable[1].neighbor.addr_64b[1]       = 0x15;
   cellTable[1].neighbor.addr_64b[2]       = 0x92;
   cellTable[1].neighbor.addr_64b[3]       = 0x09;
   cellTable[1].neighbor.addr_64b[4]       = 0x02;
   cellTable[1].neighbor.addr_64b[5]       = 0x2b;
   cellTable[1].neighbor.addr_64b[6]       = 0x00;
   if (idmanager_getMyID(ADDR_16B)->addr_16b[1]==DEBUG_MOTEID_MASTER) {
      cellTable[1].neighbor.addr_64b[7]    = DEBUG_MOTEID_SLAVE;
   } else {
      cellTable[1].neighbor.addr_64b[7]    = DEBUG_MOTEID_MASTER;
   }
   //slot 2 RX@MASTER, TX@SLAVE
   if (idmanager_getMyID(ADDR_16B)->addr_16b[1]==DEBUG_MOTEID_MASTER) {
      cellTable[2].type                    = CELLTYPE_RX;
   } else {
      cellTable[2].type                    = CELLTYPE_TX;
   }
   cellTable[2].neighbor.type              = ADDR_64B;
   cellTable[2].neighbor.addr_64b[0]       = 0x14;
   cellTable[2].neighbor.addr_64b[1]       = 0x15;
   cellTable[2].neighbor.addr_64b[2]       = 0x92;
   cellTable[2].neighbor.addr_64b[3]       = 0x09;
   cellTable[2].neighbor.addr_64b[4]       = 0x02;
   cellTable[2].neighbor.addr_64b[5]       = 0x2b;
   cellTable[2].neighbor.addr_64b[6]       = 0x00;
   if (idmanager_getMyID(ADDR_16B)->addr_16b[1]==DEBUG_MOTEID_MASTER) {
      cellTable[2].neighbor.addr_64b[7]    = DEBUG_MOTEID_SLAVE;
   } else {
      cellTable[2].neighbor.addr_64b[7]    = DEBUG_MOTEID_MASTER;
   }
   //slot 3 TX@MASTER, OFF@SLAVE
   if (idmanager_getMyID(ADDR_16B)->addr_16b[1]==DEBUG_MOTEID_MASTER) {
      cellTable[3].type                    = CELLTYPE_TX;
      cellTable[3].neighbor.type           = ADDR_64B;
      cellTable[3].neighbor.addr_64b[0]    = 0x14;
      cellTable[3].neighbor.addr_64b[1]    = 0x15;
      cellTable[3].neighbor.addr_64b[2]    = 0x92;
      cellTable[3].neighbor.addr_64b[3]    = 0x09;
      cellTable[3].neighbor.addr_64b[4]    = 0x02;
      cellTable[3].neighbor.addr_64b[5]    = 0x2b;
      cellTable[3].neighbor.addr_64b[6]    = 0x00;
      cellTable[3].neighbor.addr_64b[7]    = DEBUG_MOTEID_SLAVE;
   } else {
      cellTable[3].type                    = CELLTYPE_OFF;
   }
   //slot 4 RX@MASTER, OFF@SLAVE
   if (idmanager_getMyID(ADDR_16B)->addr_16b[1]==DEBUG_MOTEID_MASTER) {
      cellTable[4].type                    = CELLTYPE_RX;
      cellTable[4].neighbor.type           = ADDR_64B;
      cellTable[4].neighbor.addr_64b[0]    = 0x14;
      cellTable[4].neighbor.addr_64b[1]    = 0x15;
      cellTable[4].neighbor.addr_64b[2]    = 0x92;
      cellTable[4].neighbor.addr_64b[3]    = 0x09;
      cellTable[4].neighbor.addr_64b[4]    = 0x02;
      cellTable[4].neighbor.addr_64b[5]    = 0x2b;
      cellTable[4].neighbor.addr_64b[6]    = 0x00;
      cellTable[4].neighbor.addr_64b[7]    = DEBUG_MOTEID_SLAVE;
   } else {
      cellTable[4].type                    = CELLTYPE_OFF;
   }
   //slot 5 is transmit slot to neighbor 0xffffffffffffffff
   cellTable[5].type                       = CELLTYPE_TX;
   cellTable[5].neighbor.type              = ADDR_64B;
   cellTable[5].neighbor.addr_64b[0]       = 0xff;
   cellTable[5].neighbor.addr_64b[1]       = 0xff;
   cellTable[5].neighbor.addr_64b[2]       = 0xff;
   cellTable[5].neighbor.addr_64b[3]       = 0xff;
   cellTable[5].neighbor.addr_64b[4]       = 0xff;
   cellTable[5].neighbor.addr_64b[5]       = 0xff;
   cellTable[5].neighbor.addr_64b[6]       = 0xff;
   cellTable[5].neighbor.addr_64b[7]       = 0xff;
   //slot 6 is serialRx
   cellTable[6].type                       = CELLTYPE_SERIALRX;
}

cellType_t schedule_getType(asn_t asn_param) {
   uint16_t slotOffset;
   slotOffset = asn_param%SCHEDULELENGTH;
   return schedule_vars.cellTable[slotOffset].type;
}

channelOffset_t schedule_getChannelOffset(asn_t asn_param) {
   uint16_t slotOffset;
   slotOffset = asn_param%SCHEDULELENGTH;
   return schedule_vars.cellTable[slotOffset].channelOffset;
}

void schedule_getNeighbor(asn_t asn_param, open_addr_t* addrToWrite) {
   uint16_t slotOffset;
   slotOffset = asn_param%SCHEDULELENGTH;
   memcpy(addrToWrite,&schedule_vars.cellTable[slotOffset].neighbor,sizeof(open_addr_t));
}

void schedule_indicateUse(asn_t asn, bool ack){
   uint16_t slotOffset;
   
   slotOffset = asn%SCHEDULELENGTH;
   if (schedule_vars.cellTable[slotOffset].numUsed==0xFF) {
      schedule_vars.cellTable[slotOffset].numUsed/=2;
      schedule_vars.cellTable[slotOffset].numTxACK/=2;
   }
   schedule_vars.cellTable[slotOffset].numUsed++;
   if (ack==TRUE) {
      schedule_vars.cellTable[slotOffset].numTxACK++;
   }
   schedule_vars.cellTable[slotOffset].timestamp=asn;
}

bool schedule_debugPrint() {
   debugCellUsageInformation_t temp;
   schedule_vars.debugPrintRow = (schedule_vars.debugPrintRow+1)%SCHEDULELENGTH;
   temp.row        = schedule_vars.debugPrintRow;
   temp.cellUsage  = schedule_vars.cellTable[schedule_vars.debugPrintRow];
   openserial_printStatus(STATUS_SCHEDULE_CELLTABLE,
                          (uint8_t*)&temp,
                          sizeof(debugCellUsageInformation_t));
   return TRUE;
}

//=========================== private =========================================