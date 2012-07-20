#include "openwsn.h"
#include "reservation.h"
#include "res.h"
#include "schedule.h"
#include "IEEE802154E.h"
#include "ResSchedule.h"
#include "packetfunctions.h"
#include "openrandom.h"
#include "scheduler.h"
#include "opentimers.h"
#include "iphc.h"

#include "stdio.h"

//=========================== variables =======================================
typedef struct {
   uint16_t             periodMaintenance;
   open_addr_t          ResNeighborAddr;
   uint8_t              ResNumOfCells;
   uint8_t              NumOfBusyCheck;
   uint8_t              TotalNumOfBusyCheck;
   reservation_state_t  State;
   uint8_t              MacMgtTaskCounter;
   opentimer_id_t       timerId;
   reservation_granted_cbt reservationGrantedCb;
   reservation_granted_cbt reservationFailedCb;
} reservation_vars_t;

typedef struct {
   open_addr_t         NeighborAddr;
   uint16_t             Seed;
   uint8_t              RxBitMap[4];                  // 1:used, 0: potential, but not used yet.
   uint8_t              Used;                         // 1: used, 0: not used
} reservation_NeighborRxUsage_t;

typedef struct {
    uint16_t      Seed;
    uint8_t       RxBitMap[4];
    ResScheduleEntry_t  ResScheduleEntry[MAXRXCELL];
} reservation_SelfRxUsage_t;

reservation_vars_t              reservation_vars;
reservation_NeighborRxUsage_t   reservation_NeighborRxUsage_vars[MAXNUMNEIGHBORS];
reservation_SelfRxUsage_t       reservation_SelfRxUsage_vars;

typedef struct {
    uint8_t             TxCandidateBitMap[4];
    ResScheduleEntry_t  ResScheduleEntry[MAXRXCELL];
} TxCandidateList_t;

TxCandidateList_t   TxCandidateList_vars;

typedef struct {
    ResScheduleEntry_t  SortedCheckListEntry[MAXRXCELL];
    uint8_t             NumOfCells;
} SortedCheckList_t;

SortedCheckList_t   SortedCheckList_vars;

typedef struct {
  open_addr_t       NeighborAddr;
  uint8_t           NumOfRemoveCells;
  uint8_t           RemoveBitMap[4];
} RemoveList_t;

RemoveList_t   RemoveList_vars;



//=========================== prototypes ======================================

void  sendRes(open_addr_t * NeighborAddr, uint8_t* ResCommand);
void  GenSelfRxUsage(void);
void  GenTxCandidate(open_addr_t * NeighborAddr, TxCandidateList_t * pTxCandidateList);
void  GenNeighborRxCellList(ResScheduleEntry_t *pTxCandidateEnrty,uint16_t Seed);
void  CheckOtherNeighborRx(TxCandidateList_t* pTxCadidateList,uint8_t DestIndex,ResScheduleEntry_t* pResScheduleEntry);
bool  CheckSelfUsage(ResScheduleEntry_t pResScheduleEntry);
void  SortBusyCheckList();
void  ChangeState(reservation_state_t state);
void  CommandDrivenActivity(open_addr_t* SrcAddr, uint8_t* Command);
void  SendDoneDrivenActivity(void);
void  BusyCheckDoneDrivenActivity();
uint8_t  GenTxRemoveList(open_addr_t* NeighborAddr,uint8_t RequiredNumOfCell,TxCandidateList_t * pTxCandidateList);
void  ChangeState(reservation_state_t newstate);
void reservation_timer_cb();

uint8_t CountOfOne(uint8_t* BitMap);
void  ClearBit_8(uint8_t index, uint8_t* BitMap_8);
void  SetBit_8(uint8_t index, uint8_t* BitMap_8);
bool  TestSet_8 (uint8_t index, uint8_t BitMap_8);
uint8_t CondClearBit_8(uint8_t NumOfSet, uint8_t* BitMap_8);
void  ClearBit_32(uint8_t index, uint8_t * BitMap);
void  SetBit_32(uint8_t index, uint8_t * BitMap);
bool  TestSet_32(uint8_t index, uint8_t * BitMap);
void  ClearExtraBits(uint8_t NumOfCells, uint8_t * BitMap);
uint8_t decode(uint8_t d);
void RegisterToSchedule(cellType_t CellType);
void RemoveFromSchedule(open_addr_t* NeighborAddr, cellType_t CellType);

void  activity_Commandi1(open_addr_t* SrcAddr, uint8_t* Command); 
void  activity_Commandi2(open_addr_t* SrcAddr, uint8_t* Command); 
void  activity_CommandSendDonei1();
void  activity_CommandSendDonei2();
void  activity_CommandSendDonei3();
void  activity_BusyCheckDonei1();
void  activity_BusyCheckDonei2();

//temp var for reservation testing
    uint8_t LinkRequest_flag = 1;
    uint8_t AddedCellNum = 0;


//=========================== public ==========================================

void reservation_init() {
        
      uint8_t   i;
      
        // reset local variables to 0, i.e. Seed=0, means Seed hasn't been generated 
        // Seed will be generated at the first time ADV is sent out
	memset(&reservation_SelfRxUsage_vars,0,sizeof(reservation_SelfRxUsage_t));
        
        for (i=0;i<MAXNUMNEIGHBORS;i++){
	    //reset NeighborRxUsage, Seed=0  
            memset(&reservation_NeighborRxUsage_vars[i],0, sizeof(reservation_NeighborRxUsage_t));
	}
        
        //for testing
       /* reservation_vars.MacMgtTaskCounter = 0;
        reservation_vars.periodMaintenance = 1700*3+(openrandom_get16b()&0xff);         // fires every 1 sec on average
        reservation_vars.timerId = opentimers_start(reservation_vars.periodMaintenance,
                                                    TIMER_PERIODIC,TIME_MS,
                                                    reservation_timer_cb);
        
       */ 
    return;
}


// for sending ADV
// fill the ADV
void reservation_GetSeedAndBitMap(uint8_t* Seed, uint8_t* BitMap) {
    
    //generate SelfRxUsage, fill 
    if (reservation_SelfRxUsage_vars.Seed==0){
          GenSelfRxUsage();
    }
 
    Seed[0]=reservation_SelfRxUsage_vars.Seed & 0xff;
    Seed[1]=reservation_SelfRxUsage_vars.Seed/256 & 0xff;
    BitMap[0]= reservation_SelfRxUsage_vars.RxBitMap[0];
    BitMap[1]= reservation_SelfRxUsage_vars.RxBitMap[1];
    BitMap[2]= reservation_SelfRxUsage_vars.RxBitMap[2];
    BitMap[3]= reservation_SelfRxUsage_vars.RxBitMap[3];
    
}

//for receiving ADV
//record Seed and BitMap from the Neighbor's Adv
void reservation_RecordSeedAndBitMap(OpenQueueEntry_t* msg) {
    
    uint16_t      Seed;
    uint8_t       BitMap[4];
    uint8_t       i;
    bool          NewNeighbor;
    
    NewNeighbor =TRUE;
    Seed = msg->payload[6]*256 + msg->payload[5];
    BitMap[0] = msg-> payload[7];
    BitMap[1] = msg-> payload[8];
    BitMap[2] = msg-> payload[9];
    BitMap[3] = msg-> payload[10];
    
    //printf("BitMap: %x %x %x %x",BitMap[0],BitMap[1],BitMap[2],BitMap[3]);
 /*
    printf("%d \n",msg->length);
      for(uint8_t j=0; j<msg->length;j++)  
     printf("%x ",msg->payload[j]);
     printf("\n");
   */ 
    //update reservation_NeighborRxUsage_var of existing one
    for (i=0; i<MAXNUMNEIGHBORS; i++) {
      if (packetfunctions_sameAddress(&msg->l2_nextORpreviousHop, &reservation_NeighborRxUsage_vars[i].NeighborAddr)&& reservation_NeighborRxUsage_vars[i].Used){
          NewNeighbor = FALSE;
          reservation_NeighborRxUsage_vars[i].Seed = Seed;
          //memcpy(&reservation_NeighborRxUsage_vars[i].RxBitMap, &BitMap, sizeof(4));
          reservation_NeighborRxUsage_vars[i].RxBitMap[0]=BitMap[0];
          reservation_NeighborRxUsage_vars[i].RxBitMap[1]=BitMap[1];
          reservation_NeighborRxUsage_vars[i].RxBitMap[2]=BitMap[2];
          reservation_NeighborRxUsage_vars[i].RxBitMap[3]=BitMap[3];
          break;
      }       
    }
    
/*
    printf("seed=: %d\nBitMap=:",Seed);
    for(uint8_t j=0;j<4;j++)
      printf("%x ",BitMap[j]);
    printf("\nNeibor=:");
    for(uint8_t j=0;j<8;j++)
      printf("%x ",msg->l2_nextORpreviousHop.addr_64b[j]);
   */
    //if it is not existing, then, add one
    if (NewNeighbor){
      for (i=0; i<MAXNUMNEIGHBORS; i++) {
        if (reservation_NeighborRxUsage_vars[i].Used ==0) {
          reservation_NeighborRxUsage_vars[i].Seed = Seed;
          //memcpy(reservation_NeighborRxUsage_vars[i].RxBitMap, BitMap, sizeof(4));
          reservation_NeighborRxUsage_vars[i].RxBitMap[0]=BitMap[0];
          reservation_NeighborRxUsage_vars[i].RxBitMap[1]=BitMap[1];
          reservation_NeighborRxUsage_vars[i].RxBitMap[2]=BitMap[2];
          reservation_NeighborRxUsage_vars[i].RxBitMap[3]=BitMap[3];
          memcpy(&reservation_NeighborRxUsage_vars[i].NeighborAddr,&msg->l2_nextORpreviousHop,sizeof(open_addr_t));
          reservation_NeighborRxUsage_vars[i].Used=1;
          /*
          printf("i= %d\n",i);
          printf("\nseed=: %d\nBitMap=:",reservation_NeighborRxUsage_vars[i].Seed);
          for(uint8_t j=0;j<4;j++)
            printf("%x ",reservation_NeighborRxUsage_vars[i].RxBitMap[j]);
          printf("\nNeibor=:");
          for(uint8_t j=0;j<8;j++)
            printf("%x ",reservation_NeighborRxUsage_vars[i].NeighborAddr.addr_64b[j]);
          */
          
          break;
        }
      }
    }
    
    packetfunctions_tossHeader(msg,ADV_PAYLOAD_LENGTH);
    //free up the RAM
     openqueue_freePacketBuffer(msg); 
}


// when reservation command is received
void reservation_CommandReceive(OpenQueueEntry_t* msg) {
    uint8_t ResCommand[6];
    open_addr_t SrcAddr;
    
     memcpy(&SrcAddr,&msg->l2_nextORpreviousHop,sizeof(open_addr_t));
     
    //paylaod[0] equals to 0, used to distinguish from Iphc frame
    ResCommand[0]=msg->payload[1];
    if ((ResCommand[0]==RES_CELL_REQUEST)
        ||(ResCommand[0]==REMOVE_CELL_REQUEST)
        ||(ResCommand[0]==RES_CELL_RESPONSE)){
        ResCommand[1]= msg->payload[2];
        ResCommand[2]= msg->payload[3];
        ResCommand[3]= msg->payload[4];
        ResCommand[4]= msg->payload[5];
        ResCommand[5]= msg->payload[6];
     }
     //free up the RAM
     openqueue_freePacketBuffer(msg);
    
    //call command driven statemachine
    CommandDrivenActivity(&SrcAddr,ResCommand);
}



// This is one of MAC management function, but implemented with IEEE154_TYPE_DATA
// send an reservation command
void sendRes(open_addr_t * NeighborAddr, uint8_t* ResCommand) {
   OpenQueueEntry_t* ResPkt;
   
          // get a free packet buffer
         ResPkt = openqueue_getFreePacketBuffer(COMPONENT_RES);
         if (ResPkt==NULL) {
            openserial_printError(COMPONENT_RES,ERR_NO_FREE_PACKET_BUFFER,
                                  (errorparameter_t)0,
                                  (errorparameter_t)0);
            return;
         }
         
         // declare ownership over that packet
         ResPkt->creator = COMPONENT_RES;
         ResPkt->owner   = COMPONENT_RES;
         
         // reserve space for Res command
         packetfunctions_reserveHeaderSize(ResPkt, RES_PAYLOAD_LENGTH);
         // fill Res command, 1B: =0, distinguish from Iphc frame, 1B: opcode, 4B: parameter, 1B:NumOfCell
         ResPkt->payload[0] = 0;                  //distinguish from Iphc frame
         ResPkt->payload[1] = ResCommand[0];
         ResPkt->payload[2] = ResCommand[1];
         ResPkt->payload[3] = ResCommand[2];
         ResPkt->payload[4] = ResCommand[3];
         ResPkt->payload[5] = ResCommand[4];
         ResPkt->payload[6] = ResCommand[5];
         
         // some l2 information about this packet
         ResPkt->l2_frameType = IEEE154_TYPE_DATA;
         memcpy(&(ResPkt->l2_nextORpreviousHop),NeighborAddr,sizeof(open_addr_t));
                  
         // put in queue for MAC to handle
         res_send_internal(ResPkt);
}

//after reservation command is sent out
//in res.c, E_SUCCESS has been checked
void reservation_CommandSendDone(OpenQueueEntry_t* msg) {

    //if S_REMOVECELLREQUEST, then send confirm to upper layer 
    SendDoneDrivenActivity();
    
    //free up the RAM
     openqueue_freePacketBuffer(msg);   
}

//from lower layer
void reservation_IndicateBusyCheck() {
    
    BusyCheckDoneDrivenActivity();
}

//from upper layer

void reservation_LinkRequest(open_addr_t* NeighborAddr, uint8_t NumOfCell) {
  
    //printf("%d",NeighborAddr->type);  
    //according to NeighborRxUsage, ActiveSlot in schedule, 
    GenTxCandidate(NeighborAddr,&TxCandidateList_vars);
    
    //Sort the slots needed busy check from biggest number to smallest number
    SortBusyCheckList();
    
    // fill Busy Ceck List in ResSchedule
    if (SortedCheckList_vars.NumOfCells >0) {
        ResSchedule_SetBusyCheckList(&SortedCheckList_vars.SortedCheckListEntry[0], SortedCheckList_vars.NumOfCells);
        reservation_vars.NumOfBusyCheck = SortedCheckList_vars.NumOfCells;
        reservation_vars.TotalNumOfBusyCheck = SortedCheckList_vars.NumOfCells;
    } else {
        //report
        iphc_NewLinkConfirm(NeighborAddr,0);
        LinkRequest_flag = 1;    //for testing
        P2OUT ^= 0x01;
        return;
    }
    
    //change state to TX busy check state
    memcpy(&reservation_vars.ResNeighborAddr,NeighborAddr,sizeof(open_addr_t));
    reservation_vars.ResNumOfCells = NumOfCell;
    
    ChangeState(S_TXBUSYCHECK);
}


void reservation_RemoveLinkRequest(open_addr_t* NeighborAddr, uint8_t RequiredNumOfCell) {
    
    uint8_t   NumOfRemove;
    uint8_t   ResCommand[6];
    
    NumOfRemove= GenTxRemoveList(NeighborAddr,RequiredNumOfCell,&TxCandidateList_vars);
    if (NumOfRemove != RequiredNumOfCell) {
        //report ERROR
      
        //end of RemoveLinkRequest
        return;
    } else {
        // send remove link command
         ResCommand[0] = REMOVE_CELL_REQUEST;
         ResCommand[1] = RemoveList_vars.RemoveBitMap[0];
         ResCommand[2] = RemoveList_vars.RemoveBitMap[1];
         ResCommand[3] = RemoveList_vars.RemoveBitMap[2];
         ResCommand[4] = RemoveList_vars.RemoveBitMap[3];
         ResCommand[5] = RemoveList_vars.NumOfRemoveCells & 0xff;         
         
         //send out remove link command
         sendRes(NeighborAddr, ResCommand);
      
        //change state to WAITREMOVECONFIRM
        memcpy(&reservation_vars.ResNeighborAddr,NeighborAddr,sizeof(open_addr_t));
        reservation_vars.ResNumOfCells = RequiredNumOfCell;
        ChangeState(S_SENDOUTREMOVECELLREQUEST);
    }
    
    return;
}

void reservation_setcb(reservation_granted_cbt reservationGrantedCb,reservation_granted_cbt reservationFailedCb){
   reservation_vars.reservationGrantedCb = reservationGrantedCb;
   reservation_vars.reservationFailedCb = reservationFailedCb;
}

//================== private ================================

// Based on the collected NeighborRxUsage, generate SelfRxUsage
void GenSelfRxUsage() {
    uint16_t  seed, Cell, RxSlot, RxChannel;
    uint16_t  NextCellIndex;
    uint8_t   conflict;
    uint8_t   i,j;
    
    while(1){
        seed = openrandom_get16b()%((SUPERFRAMELENGTH-1)*NUMOFCHANNEL);
        if (seed%2 ==0 ) {
            seed =seed+1;
        }
        conflict=0;
        for (i=0;i<MAXNUMNEIGHBORS;i++){
          if (seed == reservation_NeighborRxUsage_vars[i].Seed) {
              conflict++;
          }
	}
        
        if (conflict ==0 ){
          break;
        }
    }
    
    NextCellIndex=1;
    for (i=0;i<MAXRXCELL;i++) {
      while(1) {
        Cell = seed * NextCellIndex % ((SUPERFRAMELENGTH-1)*NUMOFCHANNEL);
        RxSlot = Cell / NUMOFCHANNEL;
        RxChannel = Cell % NUMOFCHANNEL;
        
        conflict=0;
        for (j=0; j<i; j++) {
          if (RxSlot == reservation_SelfRxUsage_vars.ResScheduleEntry[j].SlotOffset) {
              conflict++;
          }
        }
        if (conflict ==0) {
            reservation_SelfRxUsage_vars.ResScheduleEntry[i].SlotOffset = RxSlot;
            reservation_SelfRxUsage_vars.ResScheduleEntry[i].ChannelOffset = (uint8_t)RxChannel;
            NextCellIndex++;
            break;
        } else {
            NextCellIndex++;
        }
      }         
    }   
    reservation_SelfRxUsage_vars.Seed = seed;
    reservation_SelfRxUsage_vars.RxBitMap[0]=0;
    if (MAXRXCELL>8){ 
      reservation_SelfRxUsage_vars.RxBitMap[1]=0;
    }
    else {
      reservation_SelfRxUsage_vars.RxBitMap[1]=0xff;
    }
    if (MAXRXCELL>16){
      reservation_SelfRxUsage_vars.RxBitMap[2]=0;
    }
    else {
      reservation_SelfRxUsage_vars.RxBitMap[2]=0xff;
    }
    if (MAXRXCELL>24) {
      reservation_SelfRxUsage_vars.RxBitMap[3]=0;
    }
    else {
      reservation_SelfRxUsage_vars.RxBitMap[3]=0xff;
    }
}

//reservation state machine
// called when receiving reservation command
void CommandDrivenActivity(open_addr_t* SrcAddr, uint8_t* Command) {
    
    switch (reservation_vars.State) {
         case S_IDLE:
            activity_Commandi1(SrcAddr,Command);          //command: RES_CELL_REQUEST or REMOVE_CELL_REQUEST
            break;
         case S_WAITFORRESPONSE:
            activity_Commandi2(SrcAddr,Command);         //command: RES_CELL_RESPONSE
            break;
         default:
            // log the error
            //openserial_printError(COMPONENT_IEEE802154E,ERR_WRONG_STATE_IN_ENDOFFRAME,
            //                      (errorparameter_t)ieee154e_vars.state,
            //                      (errorparameter_t)ieee154e_vars.slotOffset);
            break;
      }
    return; 
}

// called when reservation command  is SendDone successfully
void SendDoneDrivenActivity() {
      
      switch (reservation_vars.State) {
         case S_SENDOUTRESCELLREQUEST:
           activity_CommandSendDonei1();          //command: RES_CELL_REQUEST
            break;
         case S_SENDOUTRESCELLRESPONSE:
           activity_CommandSendDonei2();         //command: RES_CELL_RESPONSE
            break;
         case S_SENDOUTREMOVECELLREQUEST:
           activity_CommandSendDonei3();         //command:  REMOVE_CELL_REQUEST
           break;
         default:
            // log the error
            //openserial_printError(COMPONENT_IEEE802154E,ERR_WRONG_STATE_IN_ENDOFFRAME,
            //                      (errorparameter_t)ieee154e_vars.state,
            //                      (errorparameter_t)ieee154e_vars.slotOffset);
            break;
      }
    return;   
}

//called when one Busy Check is finished
void BusyCheckDoneDrivenActivity() {
  
    switch (reservation_vars.State) {
         case S_TXBUSYCHECK:
           activity_BusyCheckDonei1();         //busy check for identifying TxTX conflict in Rx side
            break;
         case S_HIDENCHECK:
           activity_BusyCheckDonei2();         //busy check for identifying hiden terminal in Rx side
            break;
         default:
            // log the error
            //openserial_printError(COMPONENT_IEEE802154E,ERR_WRONG_STATE_IN_ENDOFFRAME,
            //                      (errorparameter_t)ieee154e_vars.state,
            //                      (errorparameter_t)ieee154e_vars.slotOffset);
            break;
      }
    return; 
}

void ChangeState(reservation_state_t newstate) {
   // update the state
   reservation_vars.State = newstate;
   
   // wiggle the FSM debug pin
   switch (reservation_vars.State) {
      case S_IDLE: 
      case S_TXBUSYCHECK: 
      case S_SENDOUTRESCELLREQUEST:
      case S_WAITFORRESPONSE:
      case S_SENDOUTRESCELLRESPONSE:
      case S_HIDENCHECK:
      case S_SENDOUTREMOVECELLREQUEST:
         break;
   }
}

//Receive RES_CELL_REQUEST or REMOVE_CELL_REQUEST command
void  activity_Commandi1(open_addr_t * SrcAddr, uint8_t* Command) {
  uint8_t       NumOfRemoveCells;
  uint8_t       i;   
  
  
  if (Command[0]== RES_CELL_REQUEST) {
        P2OUT ^= 0x01;
        reservation_vars.ResNumOfCells = Command[5];
        
        //set TxCandidateList

        TxCandidateList_vars.TxCandidateBitMap[0]=Command[1];
        TxCandidateList_vars.TxCandidateBitMap[1]=Command[2];
        TxCandidateList_vars.TxCandidateBitMap[2]=Command[3];
        TxCandidateList_vars.TxCandidateBitMap[3]=Command[4];
        
        for (i=0; i<MAXRXCELL; i++) {
          TxCandidateList_vars.ResScheduleEntry[i].SlotOffset = reservation_SelfRxUsage_vars.ResScheduleEntry[i].SlotOffset;
          TxCandidateList_vars.ResScheduleEntry[i].ChannelOffset = reservation_SelfRxUsage_vars.ResScheduleEntry[i].ChannelOffset;
        }
        
        //self check with schedule.c
        for (i=0; i<MAXRXCELL; i++) {
          if (TestSet_32(i,&TxCandidateList_vars.TxCandidateBitMap[0])&&
                (CheckSelfUsage(TxCandidateList_vars.ResScheduleEntry[i]))){
                  ClearBit_32(i,&TxCandidateList_vars.TxCandidateBitMap[0]);
          }
        }
    
        //hiden check
        //Sort the slots needed busy check from biggest number to smallest number
        SortBusyCheckList();
    
        // fill Busy Ceck List in ResSchedule
        if (SortedCheckList_vars.NumOfCells >0) {
            ResSchedule_SetBusyCheckList(&SortedCheckList_vars.SortedCheckListEntry[0], SortedCheckList_vars.NumOfCells);
            reservation_vars.NumOfBusyCheck = SortedCheckList_vars.NumOfCells;
            reservation_vars.TotalNumOfBusyCheck = SortedCheckList_vars.NumOfCells;
            //change state to TX busy check state
            memcpy(&reservation_vars.ResNeighborAddr,SrcAddr,sizeof(open_addr_t));
            ChangeState(S_HIDENCHECK);
        } else {
            //report error
        }    
  } else if (Command[0] == REMOVE_CELL_REQUEST) {
        P2OUT ^= 0x01;
        //set RemoveCellList into TxCandidateList

        TxCandidateList_vars.TxCandidateBitMap[0]=Command[1];
        TxCandidateList_vars.TxCandidateBitMap[1]=Command[2];
        TxCandidateList_vars.TxCandidateBitMap[2]=Command[3];
        TxCandidateList_vars.TxCandidateBitMap[3]=Command[4];
                    
        for (i=0; i<MAXRXCELL; i++) {
           TxCandidateList_vars.ResScheduleEntry[i]= reservation_SelfRxUsage_vars.ResScheduleEntry[i];
        }
        
        // remove cells from schelde.c
        RemoveFromSchedule(SrcAddr, CELLTYPE_RX);
    
        //register to SelfRxUsage
         reservation_SelfRxUsage_vars.RxBitMap[0] ^= TxCandidateList_vars.TxCandidateBitMap[0];
         reservation_SelfRxUsage_vars.RxBitMap[1] ^= TxCandidateList_vars.TxCandidateBitMap[1];
         reservation_SelfRxUsage_vars.RxBitMap[2] ^= TxCandidateList_vars.TxCandidateBitMap[2];
         reservation_SelfRxUsage_vars.RxBitMap[3] ^= TxCandidateList_vars.TxCandidateBitMap[3];
          
        // notify upper layer
        NumOfRemoveCells  =CountOfOne(&TxCandidateList_vars.TxCandidateBitMap[0]);
        iphc_RemoveLinkIndicate(SrcAddr,NumOfRemoveCells);
      
        //change state
        P2OUT ^= 0x01;
        ChangeState(S_IDLE);
    
  } else {
      //do nothing
  }      
}

//receive RES_CELL_RESPONSE
void  activity_Commandi2(open_addr_t* SrcAddr, uint8_t* Command){
  // if reservation is successful, i.e. Command[5] !=0
  if (Command[5] > 0){
     //register on schedule.c
    TxCandidateList_vars.TxCandidateBitMap[0] =Command[1];
    TxCandidateList_vars.TxCandidateBitMap[1] =Command[2];
    TxCandidateList_vars.TxCandidateBitMap[2] =Command[3];
    TxCandidateList_vars.TxCandidateBitMap[3] =Command[4];
        
    RegisterToSchedule(CELLTYPE_TX);
    
    //for testing
    LinkRequest_flag = 1;
    AddedCellNum ++;
    P2OUT ^= 0x01;
    
    // notify upper layer
    iphc_NewLinkConfirm(&reservation_vars.ResNeighborAddr, reservation_vars.ResNumOfCells);    
    //call the callback:
   scheduler_push_task(reservation_vars.reservationGrantedCb,TASKPRIO_RESERVATION);
  } else {
      //for testing
      scheduler_push_task(reservation_vars.reservationFailedCb,TASKPRIO_RESERVATION);
      LinkRequest_flag = 1;
      P2OUT ^= 0x01;
      iphc_NewLinkConfirm(&reservation_vars.ResNeighborAddr, 0); 
  }
  ChangeState(S_IDLE);
}

//RES_CELL_REQUEST is SendDond successfully
void  activity_CommandSendDonei1(){
    //nothing to do
    ChangeState(S_WAITFORRESPONSE);  
    return;
}

//RES_CELL_RESPONSE is SendDond successfully
void  activity_CommandSendDonei2(){
    //nothing to do 
    P2OUT ^= 0x01;
    ChangeState(S_IDLE);
    return;
}

//REMOVE_CELL_REQUEST is SendDond successfully
void  activity_CommandSendDonei3(){
    //remove cells from schedule
    RemoveFromSchedule(&reservation_vars.ResNeighborAddr,CELLTYPE_TX);  
    
    //register to SelfRxUsage 
    reservation_SelfRxUsage_vars.RxBitMap[0] ^= TxCandidateList_vars.TxCandidateBitMap[0];
    reservation_SelfRxUsage_vars.RxBitMap[1] ^= TxCandidateList_vars.TxCandidateBitMap[1];
    reservation_SelfRxUsage_vars.RxBitMap[2] ^= TxCandidateList_vars.TxCandidateBitMap[2];
    reservation_SelfRxUsage_vars.RxBitMap[3] ^= TxCandidateList_vars.TxCandidateBitMap[3];
    
    //for testing
    P2OUT ^= 0x01;
    if(AddedCellNum > 0)
    AddedCellNum --;
    LinkRequest_flag = 1;
    
    //notify upper layer
    iphc_RemoveLinkConfirm(&RemoveList_vars.NeighborAddr,RemoveList_vars.NumOfRemoveCells);
    
    ChangeState(S_IDLE);
}

//During TxTX check
void  activity_BusyCheckDonei1(){
    
    uint8_t index, i,j;
    uint8_t NumOfCandidate;
    uint8_t ResCommand[6];
    uint16_t Slot;

      reservation_vars.NumOfBusyCheck--;
    
      //when busy check finish
      if (reservation_vars.NumOfBusyCheck ==0) {
        for (i=0; i< reservation_vars.TotalNumOfBusyCheck; i++) {
            if (ResSchedule_BusyCheckStatus(i)){
              Slot=ResSchedule_BusyCheckSlot(i);
              for (j=0; j<MAXRXCELL; j++) {
                if (Slot == TxCandidateList_vars.ResScheduleEntry[j].SlotOffset) {
                  index=j;
                  break;
              }
            }
            ClearBit_32(index, TxCandidateList_vars.TxCandidateBitMap);
          }
        }
    
       NumOfCandidate = CountOfOne (TxCandidateList_vars.TxCandidateBitMap);
        
       if (NumOfCandidate < reservation_vars.ResNumOfCells){
          iphc_NewLinkConfirm(&reservation_vars.ResNeighborAddr,0);
          ChangeState(S_IDLE);
          LinkRequest_flag =1;
          P2OUT ^= 0x01;
        } else {
          //form ResCommaned
          ResCommand[0] = RES_CELL_REQUEST;
          ResCommand[1] = TxCandidateList_vars.TxCandidateBitMap[0];
          ResCommand[2] = TxCandidateList_vars.TxCandidateBitMap[1];
          ResCommand[3] = TxCandidateList_vars.TxCandidateBitMap[2];
          ResCommand[4] = TxCandidateList_vars.TxCandidateBitMap[3];
          ResCommand[5] = reservation_vars.ResNumOfCells;         
         
          //Send out reservation request
          sendRes(&reservation_vars.ResNeighborAddr, ResCommand);
          //Change stste
          //ChangeState(S_WAITFORRESPONSE);
          ChangeState(S_SENDOUTRESCELLREQUEST);
         }
    }
}

//During Hiden check
void  activity_BusyCheckDonei2(){
    uint8_t index, i,j;
    uint8_t NumOfCandidate;
    uint8_t ResCommand[6];
    uint16_t Slot;
    
    reservation_vars.NumOfBusyCheck--;
    
    //when busy check finsih
    if (reservation_vars.NumOfBusyCheck ==0) {        
       for (i=0; i< reservation_vars.TotalNumOfBusyCheck; i++) {
            if (ResSchedule_BusyCheckStatus(i)){
              Slot=ResSchedule_BusyCheckSlot(i);
              for (j=0; j<MAXRXCELL; j++) {
                if (Slot == TxCandidateList_vars.ResScheduleEntry[j].SlotOffset) {
                  index=j;
                  break;
              }
            }
            ClearBit_32(index, TxCandidateList_vars.TxCandidateBitMap);
          }
        }
      
        NumOfCandidate = CountOfOne (TxCandidateList_vars.TxCandidateBitMap);
        
        //form ResCommaned and make registration
        ResCommand[0] = RES_CELL_RESPONSE;
        if (NumOfCandidate < reservation_vars.ResNumOfCells) {
          ResCommand[1] = 0;
          ResCommand[2] = 0;
          ResCommand[3] = 0;
          ResCommand[4] = 0;
          ResCommand[5] = 0;
        } else {
          //clear (NumOfCandidate - reservation_vars.ResNumOfCells) bits in the BitMap
          ClearExtraBits(reservation_vars.ResNumOfCells,TxCandidateList_vars.TxCandidateBitMap);
          ResCommand[1] = TxCandidateList_vars.TxCandidateBitMap[0];
          ResCommand[2] = TxCandidateList_vars.TxCandidateBitMap[1];
          ResCommand[3] = TxCandidateList_vars.TxCandidateBitMap[2];
          ResCommand[4] = TxCandidateList_vars.TxCandidateBitMap[3];
          ResCommand[5] = reservation_vars.ResNumOfCells;
          
          //register to schedule.c
          RegisterToSchedule(CELLTYPE_RX);
          
          //register to SelfRxUsage
          reservation_SelfRxUsage_vars.RxBitMap[0] ^= TxCandidateList_vars.TxCandidateBitMap[0];
          reservation_SelfRxUsage_vars.RxBitMap[1] ^= TxCandidateList_vars.TxCandidateBitMap[1];
          reservation_SelfRxUsage_vars.RxBitMap[2] ^= TxCandidateList_vars.TxCandidateBitMap[2];
          reservation_SelfRxUsage_vars.RxBitMap[3] ^= TxCandidateList_vars.TxCandidateBitMap[3];
          
          //notify upper layer
          iphc_NewLinkIndicate(&reservation_vars.ResNeighborAddr,reservation_vars.ResNumOfCells);
        }
        
        //Send out reservation response
        sendRes(&reservation_vars.ResNeighborAddr, ResCommand);
        //Change stste
        //ChangeState(S_IDLE);
        ChangeState(S_SENDOUTRESCELLRESPONSE);
    }
}

//initalize TxCandidateList with given NeigborRxUsage
//Neighbors' NeighborRxUsage check
//self check with schedule.c
void  GenTxCandidate(open_addr_t * NeighborAddr, TxCandidateList_t * pTxCandidateList) {
    uint8_t             i, index;
    ResScheduleEntry_t  NeighborRxCell[MAXRXCELL];  
  
    for (i=0; i<MAXNUMNEIGHBORS; i++){
      if (packetfunctions_sameAddress(NeighborAddr, &reservation_NeighborRxUsage_vars[i].NeighborAddr)&& reservation_NeighborRxUsage_vars[i].Used){
        //printf("\nindex= %d",i);
        index =i;
        break;
      }
    }
    //printf("\nseed= %d",reservation_NeighborRxUsage_vars[index].Seed);
    /*
    printf("%d\n",NeighborAddr->type);
    for(uint8_t j=0;j<8;j++)
    printf("%x ",NeighborAddr->addr_64b[j]);
    */
    
    //memcpy(pTxCandidateList->TxCandidateBitMap,reservation_NeighborRxUsage_vars[index].RxBitMap,4);
    pTxCandidateList->TxCandidateBitMap[0] = ~reservation_NeighborRxUsage_vars[index].RxBitMap[0];
    pTxCandidateList->TxCandidateBitMap[1] = ~reservation_NeighborRxUsage_vars[index].RxBitMap[1];
    pTxCandidateList->TxCandidateBitMap[2] = ~reservation_NeighborRxUsage_vars[index].RxBitMap[2];
    pTxCandidateList->TxCandidateBitMap[3] = ~reservation_NeighborRxUsage_vars[index].RxBitMap[3];
    /*
     printf("\n0: ");
    for(uint8_t j=0;j<4;j++)
    printf("%x ",pTxCandidateList->TxCandidateBitMap[j]);
    */
    GenNeighborRxCellList(pTxCandidateList->ResScheduleEntry,reservation_NeighborRxUsage_vars[index].Seed);
    /*
     printf("\n1: ");
        for(uint8_t j=0;j<4;j++)
    printf("%x ",pTxCandidateList->TxCandidateBitMap[j]);
    */
    //Neighbors' Rx conflict check
    for (i=0; i<MAXNUMNEIGHBORS; i++) {
      if ((i!=index) && (reservation_NeighborRxUsage_vars[i].Used)){
        GenNeighborRxCellList(&NeighborRxCell[0],reservation_NeighborRxUsage_vars[i].Seed);
        CheckOtherNeighborRx(pTxCandidateList, i, &NeighborRxCell[0]);
      }
    }
    /*
         printf("\n2:");
        for(uint8_t j=0;j<4;j++)
    printf("%x ",pTxCandidateList->TxCandidateBitMap[j]);
    */
    //self usage check
    for (i=0; i<MAXRXCELL; i++) {
             if (TestSet_32(i,&TxCandidateList_vars.TxCandidateBitMap[0])&&
                (CheckSelfUsage(TxCandidateList_vars.ResScheduleEntry[i]))){
                 ClearBit_32(i,&TxCandidateList_vars.TxCandidateBitMap[0]); 
              }
     }
    /*
    printf("\n3:");
    for(uint8_t j=0;j<4;j++)
      printf("%x ",pTxCandidateList->TxCandidateBitMap[j]);
    
    printf("\n slot  chan\n");
    for(uint8_t j=0;j<MAXRXCELL;j++)
    {
      printf(" %d %d\n",TxCandidateList_vars.ResScheduleEntry[j].SlotOffset,TxCandidateList_vars.ResScheduleEntry[j].ChannelOffset);
    }
    */
    
}



void  CheckOtherNeighborRx(TxCandidateList_t* pTxCandidateList, uint8_t DestIndex,ResScheduleEntry_t* pResScheduleEntry) {
    uint8_t i,j;

    for (i=0; i<MAXRXCELL; i++) {
      if (TestSet_32(i,&pTxCandidateList->TxCandidateBitMap[0])) {
            for (j=0; j<MAXRXCELL; j++) {
              if (TestSet_32(j,&reservation_NeighborRxUsage_vars[DestIndex].RxBitMap[0])){                   
                    if ((pResScheduleEntry[j].SlotOffset == pTxCandidateList->ResScheduleEntry[i].SlotOffset)&&
                        (pResScheduleEntry[j].ChannelOffset == pTxCandidateList->ResScheduleEntry[i].ChannelOffset)) {
                          ClearBit_32(i,&pTxCandidateList->TxCandidateBitMap[0]);
                       }
                  }
               }
          }
    }      
}

//check with the schedule.c
bool  CheckSelfUsage(ResScheduleEntry_t pResScheduleEntry){
    bool  Conflict;
    //The SlotOffset is in data plane, schedule_getFrameLength() return length of control plane
    Conflict  = schedule_IsUsedSlot(pResScheduleEntry.SlotOffset + schedule_getFrameLength());
     
    return Conflict;
}

//generate NeighborRxCellList while it is used
void  GenNeighborRxCellList(ResScheduleEntry_t *pTxCandidateList,uint16_t Seed) {// Is the param1 needed?
    uint16_t  NextCellIndex, RxSlot, Cell;
    uint8_t   i,j, RxChannel;
    uint8_t   conflict;
  
    NextCellIndex=1;
    for (i=0;i<MAXRXCELL;i++) {
      while (1) {
        Cell = Seed * NextCellIndex % ((SUPERFRAMELENGTH-1)*NUMOFCHANNEL);
        RxSlot = Cell / NUMOFCHANNEL;
        RxChannel = Cell % NUMOFCHANNEL;
        
        conflict=0;
        for (j=0; j<i; j++) {
            if (RxSlot == pTxCandidateList[j].SlotOffset) {
              conflict++;
          }
        }
        if (conflict ==0) {
            pTxCandidateList[i].SlotOffset = RxSlot;
            pTxCandidateList[i].ChannelOffset = (uint8_t)RxChannel;

            NextCellIndex++;
            break;
        } else {
            NextCellIndex++;
        }
      }         
    }
}

// compress TxCandidateList_vars into SortedCheckList_vars
void  SortBusyCheckList(){
    uint8_t   i,NumOfCells;  
    
    NumOfCells=0;
    for (i=0; i<MAXRXCELL; i++) {
      if (TestSet_32(i,&TxCandidateList_vars.TxCandidateBitMap[0])){
        SortedCheckList_vars.SortedCheckListEntry[NumOfCells].SlotOffset=TxCandidateList_vars.ResScheduleEntry[i].SlotOffset;
        SortedCheckList_vars.SortedCheckListEntry[NumOfCells].ChannelOffset=TxCandidateList_vars.ResScheduleEntry[i].ChannelOffset;
        NumOfCells++;
      }
    }
    SortedCheckList_vars.NumOfCells =NumOfCells;
    
    /*printf("\n BitMap: %x %x %x %x\n",TxCandidateList_vars.TxCandidateBitMap[0],TxCandidateList_vars.TxCandidateBitMap[1],TxCandidateList_vars.TxCandidateBitMap[2],TxCandidateList_vars.TxCandidateBitMap[3]);
    printf("\n slot  chan %d\n",NumOfCells);
    for(uint8_t j=0;j<NumOfCells;j++)
    {
      printf(" %d %d\n",SortedCheckList_vars.SortedCheckListEntry[j].SlotOffset,SortedCheckList_vars.SortedCheckListEntry[j].ChannelOffset);
    }*/   
        
    return;
}

uint8_t  GenTxRemoveList(open_addr_t* NeighborAddr,uint8_t RequiredNumOfCell,TxCandidateList_t * pTxCandidateList) {
    
    uint8_t   NumOfRemoveCell;
    uint8_t   i,index;
    //ResScheduleEntry_t  NeighborCellEntry[MAXRXCELL];
    
       
    index =MAXNUMNEIGHBORS;
    for (i=0; i<MAXNUMNEIGHBORS; i++) {
      if ((reservation_NeighborRxUsage_vars[i].Used==1)&&
          (packetfunctions_sameAddress(&reservation_NeighborRxUsage_vars[i].NeighborAddr, NeighborAddr))){
            index =i;
            break;
          }
      }
    //The Neighbor is not existing
    if (index == MAXNUMNEIGHBORS){
      return 0;
    }
  
    GenNeighborRxCellList(pTxCandidateList->ResScheduleEntry,reservation_NeighborRxUsage_vars[index].Seed);
      
    //find Cells from schedule.c
    TxCandidateList_vars.TxCandidateBitMap[0]  =0;
    TxCandidateList_vars.TxCandidateBitMap[1]  =0;
    TxCandidateList_vars.TxCandidateBitMap[2]  =0;
    TxCandidateList_vars.TxCandidateBitMap[3]  =0;
    NumOfRemoveCell =0;
    for (i=0; i<MAXRXCELL; i++){
      //The SlotOffset is in data plane, schedule_getFrameLength() return length of control plane  
      if((~TestSet_32(i,&reservation_NeighborRxUsage_vars[index].RxBitMap[0])) &&
           (schedule_IsMyCell(pTxCandidateList->ResScheduleEntry[i].SlotOffset +schedule_getFrameLength(), 
                              pTxCandidateList->ResScheduleEntry[i].ChannelOffset, 
                              CELLTYPE_TX, 
                              &reservation_NeighborRxUsage_vars[index].NeighborAddr))){
                     SetBit_32(i,&TxCandidateList_vars.TxCandidateBitMap[0]);           //used as RemoveCandidate BitMap
                     NumOfRemoveCell++;
        }
        if (NumOfRemoveCell == RequiredNumOfCell){
          break;
        }
    }    
    //memcpy(&RemoveList_vars.RemoveBitMap,&TxCandidateList_vars.TxCandidateBitMap, sizeof(4));
    RemoveList_vars.RemoveBitMap[0]= TxCandidateList_vars.TxCandidateBitMap[0]; 
    RemoveList_vars.RemoveBitMap[1]= TxCandidateList_vars.TxCandidateBitMap[1];
    RemoveList_vars.RemoveBitMap[2]= TxCandidateList_vars.TxCandidateBitMap[2];
    RemoveList_vars.RemoveBitMap[3]= TxCandidateList_vars.TxCandidateBitMap[3];
    RemoveList_vars.NumOfRemoveCells=NumOfRemoveCell;
    memcpy(&RemoveList_vars.NeighborAddr,NeighborAddr,sizeof(open_addr_t));
    
    return NumOfRemoveCell;
}



uint8_t CountOfOne(uint8_t* BitMap) {
      uint8_t   CntOfSet;
      uint8_t   c,d;
  
      CntOfSet=0;
      d= (BitMap[0]&0xff);
      for (c=0; d; c++){
        d &= d-1;
      }
      CntOfSet = CntOfSet+c;

      if (MAXRXCELL>8) {
        d= (BitMap[1]&0xff);
        for (c=0; d; c++){
          d &= d-1;
        }
        CntOfSet = CntOfSet+c;
      }
      
      if (MAXRXCELL>16){
        d= (BitMap[2]&0xff);
        for (c=0; d; c++){
          d &= d-1;
        }
        CntOfSet = CntOfSet+c;
      }
      
      if(MAXRXCELL>24){
        d= (BitMap[3]&0xff);
        for (c=0; d; c++){
          d &= d-1;
        }
        CntOfSet = CntOfSet+c;
      }
      return CntOfSet;
}

void ClearBit_32(uint8_t index, uint8_t * BitMap) {
   
    if (index <8 ){
        ClearBit_8(index, &BitMap[0]);
    } else if (index <16) {
        ClearBit_8(index-8, &BitMap[1]);
    } else if (index <24) {
        ClearBit_8(index-16, &BitMap[2]);
    } else {
        ClearBit_8(index-24, &BitMap[3]);
    }
}

void SetBit_32(uint8_t index, uint8_t * BitMap) {
   
    if (index <8 ){
        SetBit_8(index, &BitMap[0]);
    } else if (index <16) {
        SetBit_8(index-8, &BitMap[1]);
    } else if (index <24) {
        SetBit_8(index-16, &BitMap[2]);
    } else {
        SetBit_8(index-24, &BitMap[3]);
    }
}

bool TestSet_32(uint8_t index, uint8_t * BitMap) {
    bool flag; 
  
    if (index <8 ){
        flag  =TestSet_8(index, BitMap[0]);
    } else if (index <16) {
        flag  =TestSet_8(index-8, BitMap[1]);
    } else if (index <24) {
        flag  =TestSet_8(index-16, BitMap[2]);
    } else {
        flag  =TestSet_8(index-24, BitMap[3]);
    }
    return flag;
}


void ClearExtraBits(uint8_t NumOfCells, uint8_t * BitMap){
    
    NumOfCells = CondClearBit_8(NumOfCells, &BitMap[0]);
    NumOfCells = CondClearBit_8(NumOfCells, &BitMap[1]);
    NumOfCells = CondClearBit_8(NumOfCells, &BitMap[2]);
    NumOfCells = CondClearBit_8(NumOfCells, &BitMap[3]);
}

void  ClearBit_8(uint8_t index, uint8_t* BitMap_8){
    BitMap_8[0] &= ~(decode(index));
}

void  SetBit_8(uint8_t index, uint8_t* BitMap_8){
    BitMap_8[0] |= decode(index);   
}

uint8_t decode(uint8_t d) {
    switch  (d) {
      case  0:
        return 0x01;
      case  1:
        return 0x02;
      case  2:
        return 0x04;
      case  3:
        return 0x08;
      case  4:
        return 0x10;
      case  5:
        return 0x20;
      case  6:
        return 0x40;
      case  7:
        return 0x80;      
      }
    return 0;
}

uint8_t CondClearBit_8(uint8_t NumOfSet, uint8_t* BitMap_8) {
    uint8_t   i;
    
    for (i=0; i<8; i++){
      if (NumOfSet >0 && TestSet_8(i, BitMap_8[0])){
          NumOfSet--;
      } else {
          ClearBit_8(i,BitMap_8);
      }
    }   
    return NumOfSet;
}

bool  TestSet_8 (uint8_t index, uint8_t BitMap_8){
    uint8_t   d;
    
    d =BitMap_8;
    d = (d>>index)&0x01;      //>> or <<??? needs to check bitwise format
    if (d==1){
      return TRUE;
    } else {
      return FALSE;
    }    
}

void RegisterToSchedule(cellType_t CellType) {
    uint8_t i;
    
    for (i=0; i<MAXRXCELL; i++) {
      if (TestSet_32(i,&TxCandidateList_vars.TxCandidateBitMap[0])){
        //The SlotOffset is in data plane, schedule_getFrameLength() return length of control plane  
        schedule_addActiveSlot(TxCandidateList_vars.ResScheduleEntry[i].SlotOffset +schedule_getFrameLength(),
                                 CellType,
                                 0,         //not shared
                                 TxCandidateList_vars.ResScheduleEntry[i].ChannelOffset,
                                 &reservation_vars.ResNeighborAddr);
      }
    }
}

void RemoveFromSchedule(open_addr_t* NeighborAddr, cellType_t CellType) {
    uint8_t i;
    
    for (i=0; i<MAXRXCELL; i++) {
      if (TestSet_32(i,&TxCandidateList_vars.TxCandidateBitMap[0])){
        //The SlotOffset is in data plane, schedule_getFrameLength() return length of control plane  
        schedule_RemoveCell(TxCandidateList_vars.ResScheduleEntry[i].SlotOffset +schedule_getFrameLength(),
                              TxCandidateList_vars.ResScheduleEntry[i].ChannelOffset,
                              CellType,
                              NeighborAddr);
      }
    }
}

/*
//for testing
void timers_reservation_fired() {
   
      open_addr_t*      NeighAddr;
  
      NeighAddr = neighbors_GetOneNeighbor();

      if (NeighAddr!=NULL) {
          reservation_vars.MacMgtTaskCounter = (reservation_vars.MacMgtTaskCounter+1)%2;
          if (reservation_vars.MacMgtTaskCounter==0) {
            
            //reservation_RemoveLinkRequest(NeighAddr, 1);
            
          } else {
            
            if(AddedCellNum)    //if I have added one more Cell
            {
              if(LinkRequest_flag)//Binary Semaphore for LinkRequest or RemoveLinkRequest
              {
                P2OUT ^= 0x01;
                LinkRequest_flag = 0;    //for testing
                //reservation_RemoveLinkRequest(NeighAddr, 1);//remove one Cell
                reservation_LinkRequest(NeighAddr, 1);                
              }
            }
            else
            
            if(LinkRequest_flag)    //Binary Semaphore for LinkRequest or RemoveLinkRequest
            { 
              P2OUT ^= 0x01;
              LinkRequest_flag = 0;    //for testing
              reservation_LinkRequest(NeighAddr, 1);            
            }
            
          }   
      } else {
            openserial_printError(COMPONENT_RES,ERR_NO_FREE_PACKET_BUFFER,
                                  (errorparameter_t)0,
                                  (errorparameter_t)0);
      }
}

//for testing
void reservation_timer_cb() {
   scheduler_push_task(timers_reservation_fired,TASKPRIO_RESERVATION);
}
*/

//for testing
void isr_reservation_button() {
   
      open_addr_t*      NeighAddr;
  
      NeighAddr = neighbors_GetOneNeighbor();

      if (NeighAddr!=NULL) {
          reservation_vars.MacMgtTaskCounter = (reservation_vars.MacMgtTaskCounter+1)%2;
          if(reservation_vars.MacMgtTaskCounter==0)    //if I have added one more Cell
            {
             if(LinkRequest_flag)//Binary Semaphore for LinkRequest or RemoveLinkRequest
              {
                P2OUT ^= 0x01;
                LinkRequest_flag = 0;    //for testing
                reservation_RemoveLinkRequest(NeighAddr, 2);//remove one Cell              
              }
            }
          else
            if(LinkRequest_flag)    //Binary Semaphore for LinkRequest or RemoveLinkRequest
            { 
              P2OUT ^= 0x01;
              LinkRequest_flag = 0;    //for testing
              reservation_LinkRequest(NeighAddr, 6);
            }
          
      } else {
            openserial_printError(COMPONENT_RES,ERR_NO_FREE_PACKET_BUFFER,
                                  (errorparameter_t)0,
                                  (errorparameter_t)0);
      }
}
