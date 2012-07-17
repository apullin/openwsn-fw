#include "openwsn.h"
#include "schedule.h"
#include "openserial.h"
#include "idmanager.h"
#include "openrandom.h"
#include "board_info.h"

#include "ResSchedule.h"
#include "reservation.h"
#include "stdio.h"

//=========================== variables =======================================

typedef struct {
	ResScheduleEntry_t  ResScheduleBuf[MAXRXCELL];
        open_addr_t         NeighborAddr;
        slotOffset_t        BusyCheckedSlotOffset[MAXRXCELL];
        bool                BusyCheckedSlotStatus[MAXRXCELL];
	uint8_t             NumOfBusyCheck;
        uint8_t             NumOfBusyCheckDone;
} ResSchedule_vars_t;

ResSchedule_vars_t ResSchedule_vars;

uint8_t ResSchedule_TestCntBC, ResSchedule_TestCntBCFalse, ResSchedule_TestCntBCTrue;

//=========================== prototypes ======================================

void ResSchedule_resetEntry(scheduleEntry_t* pScheduleEntry);


//=========================== public ==========================================

//=== admin

void ResSchedule_init() {

	// reset local variables to 0
	memset(&ResSchedule_vars,0,sizeof(ResSchedule_vars_t));
        
        
        
        for(uint8_t i=0; i<MAXRXCELL; i++)
        {
          ResSchedule_vars.ResScheduleBuf[i].SlotOffset = -1;
        }
}


// ===== called by reservation.c
// ===== Busy Check List is sorted based on slot offset, from high to low. 
void ResSchedule_SetBusyCheckList(ResScheduleEntry_t* pResScheduleEntry, uint8_t NumBusyCheck) {
       uint8_t      i,j, index;
       uint16_t     MaxSlotNum;       
       
       memset(&ResSchedule_vars,0,sizeof(ResSchedule_vars_t));
       for (i=0; i< NumBusyCheck; i++) {
            MaxSlotNum  =0;
            index =0;
            for (j=0; j<NumBusyCheck; j++){
              if ((pResScheduleEntry[j].SlotOffset !=-1)&&(pResScheduleEntry[j].SlotOffset >= MaxSlotNum)){
                  index = j;
                  MaxSlotNum = pResScheduleEntry[j].SlotOffset;
              }
            }
            
            ResSchedule_vars.ResScheduleBuf[i].SlotOffset =pResScheduleEntry[index].SlotOffset;
            ResSchedule_vars.ResScheduleBuf[i].ChannelOffset =pResScheduleEntry[index].ChannelOffset;
            pResScheduleEntry[index].SlotOffset= -1;
       }
       ResSchedule_vars.NumOfBusyCheck=NumBusyCheck;
       ResSchedule_vars.NumOfBusyCheckDone=NumBusyCheck;
       
       ResSchedule_TestCntBC =0;
       ResSchedule_TestCntBCFalse=0;
       ResSchedule_TestCntBCTrue=0;
    /* 
    printf("\n slot  chan\n");
    for(uint8_t j=0;j<NumBusyCheck;j++)
    {
      printf(" %d %d\n",ResSchedule_vars.ResScheduleBuf[j].SlotOffset,ResSchedule_vars.ResScheduleBuf[j].ChannelOffset);
    }
    */  
}

void ResSchedule_addTestCntBC(uint8_t i) {
  if (i==1){
    ResSchedule_TestCntBC++;
  }
  if (i==2){
    ResSchedule_TestCntBCFalse++;
  }
  if (i==3){
    ResSchedule_TestCntBCTrue++;
  }
}

// ===== called by IEEE802154E.c

slotOffset_t ResSchedule_getNextBusyCheckSlotOffset() {
        slotOffset_t res;
        
        INTERRUPT_DECLARATION();
        DISABLE_INTERRUPTS();
	
        if (ResSchedule_vars.NumOfBusyCheck >0) {
              ResSchedule_vars.NumOfBusyCheck--;
              res= ResSchedule_vars.ResScheduleBuf[ResSchedule_vars.NumOfBusyCheck].SlotOffset;            
        } else {
              res=-1;
        }
        
	// return next busy check slot's slotOffset
	ENABLE_INTERRUPTS();
	return res;
}

channelOffset_t ResSchedule_getChannelOffset(slotOffset_t slotOffset) {
	channelOffset_t res;
	uint8_t i;
        
         INTERRUPT_DECLARATION();
         DISABLE_INTERRUPTS();
	for (i=0; i<MAXRXCELL; i++){
          if (slotOffset == ResSchedule_vars.ResScheduleBuf[i].SlotOffset){
              res= ResSchedule_vars.ResScheduleBuf[i].ChannelOffset;
              break;
          }
        }
	ENABLE_INTERRUPTS();
	return res;
}


void ResSchedule_getNeighbor(open_addr_t* addrToWrite) {
	 INTERRUPT_DECLARATION();
        DISABLE_INTERRUPTS();
	memcpy(addrToWrite,&(ResSchedule_vars.NeighborAddr),sizeof(open_addr_t));
	ENABLE_INTERRUPTS();
}

void ResSchedule_SetBusyCeckedResult(slotOffset_t SlotOffset, bool Busy) {
        
         INTERRUPT_DECLARATION();
        DISABLE_INTERRUPTS();            
        ResSchedule_vars.NumOfBusyCheckDone--;
        ResSchedule_vars.BusyCheckedSlotOffset[ResSchedule_vars.NumOfBusyCheckDone]= SlotOffset;
        ResSchedule_vars.BusyCheckedSlotStatus[ResSchedule_vars.NumOfBusyCheckDone]= Busy;
        ENABLE_INTERRUPTS(); 
}

slotOffset_t    ResSchedule_BusyCheckSlot(uint8_t i){
    return ResSchedule_vars.BusyCheckedSlotOffset[i];
}

uint8_t         ResSchedule_BusyCheckStatus(uint8_t i){
    return ResSchedule_vars.BusyCheckedSlotStatus[i];
}
