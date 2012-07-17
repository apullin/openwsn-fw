#include "openwsn.h"
#include "schedule.h"



//=========================== typedef =========================================


typedef struct {
   slotOffset_t         SlotOffset;
   channelOffset_t      ChannelOffset;
} ResScheduleEntry_t;

//=========================== variables =======================================

//=========================== prototypes ======================================

// admin
void              ResSchedule_init();


// from reservation.c
void            ResSchedule_SetBusyCheckList(ResScheduleEntry_t* pResScheduleEntry, uint8_t NumBusyCheck);
void            ResSchedule_SetBusyCeckedResult(slotOffset_t SlotOffset, bool Busy);
slotOffset_t    ResSchedule_BusyCheckSlot(uint8_t i);
uint8_t         ResSchedule_BusyCheckStatus(uint8_t i);

 // from IEEE802154E
 slotOffset_t     ResSchedule_getNextBusyCheckSlotOffset();
 channelOffset_t  ResSchedule_getChannelOffset(slotOffset_t slotOffset);

void ResSchedule_addTestCntBC(uint8_t i);
