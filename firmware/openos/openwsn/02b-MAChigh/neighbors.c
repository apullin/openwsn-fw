#include "openwsn.h"
#include "neighbors.h"
#include "openqueue.h"
#include "packetfunctions.h"
#include "idmanager.h"
#include "openserial.h"
#include "IEEE802154E.h"
#include "linkcost.h"

//to force routing topology -- see above.
//#define FORCE_MULTIHOP 
//#define GINA_FORCE_MULTIHOP 
//#define TELOSB_FORCE_MULTIHOP 
//=========================== variables =======================================

typedef struct {
   neighborRow_t    neighbors[MAXNUMNEIGHBORS];
   dagrank_t        myDAGrank;
   uint8_t          debugRow;
   //=================//
   uint8_t     testing;
} neighbors_vars_t;

neighbors_vars_t neighbors_vars;

//=========================== prototypes ======================================

void registerNewNeighbor(open_addr_t* neighborID,
                         int8_t       rssi,
                         asn_t*       asnTimestamp);
bool isNeighbor(open_addr_t* neighbor);
void removeNeighbor(uint8_t neighborIndex);
bool isThisRowMatching(open_addr_t* address,
                       uint8_t rowNumber);

//=========================== public ==========================================

void neighbors_init() {
   uint8_t i;
   for (i=0;i<MAXNUMNEIGHBORS;i++){
      neighbors_vars.neighbors[i].used=FALSE;
   }
   if (idmanager_getIsDAGroot()==TRUE) {
      neighbors_vars.myDAGrank=0;
   } else {
      neighbors_vars.myDAGrank=0xffff;
   }
}

void neighbors_receiveDIO(OpenQueueEntry_t* msg,icmpv6rpl_dio_t* dio) {
   uint8_t i;
 //  uint8_t temp_linkCost;
   msg->owner = COMPONENT_NEIGHBORS;
   if (isNeighbor(&(msg->l2_nextORpreviousHop))==TRUE) {
      for (i=0;i<MAXNUMNEIGHBORS;i++) {
         if (isThisRowMatching(&(msg->l2_nextORpreviousHop),i)) {
            //neighbors_vars.neighbors[i].DAGrank = *((uint8_t*)(msg->payload));
           //poipoi xv is the rang big endian??
           neighbors_vars.neighbors[i].DAGrank = dio->rank;
            // poipoi: single hop
           /* if (neighbors_vars.neighbors[i].DAGrank==0x00) {
               neighbors_vars.neighbors[i].parentPreference=MAXPREFERENCE;
               if (neighbors_vars.neighbors[i].numTxACK==0) {
                  temp_linkCost=15; //TODO: evaluate using RSSI?
               } else {
                  temp_linkCost=linkcost_calcETX(neighbors_vars.neighbors[i].numTx,neighbors_vars.neighbors[i].numTxACK);
               }
               if (idmanager_getIsDAGroot()==FALSE) {
                 neighbors_vars.myDAGrank=neighbors_vars.neighbors[i].DAGrank+temp_linkCost;
               }
            } else {
               neighbors_vars.neighbors[i].parentPreference=0;
            }*/
            break;
         }
      }
   }
   // poipoi: single hop
   neighbors_updateMyDAGrankAndNeighborPreference(); 
}

void neighbors_indicateRx(open_addr_t* l2_src,
                          int8_t       rssi,
                          asn_t*       asnTimestamp) {
   uint8_t i=0;
   while (i<MAXNUMNEIGHBORS) {
      if (isThisRowMatching(l2_src,i)) {
         neighbors_vars.neighbors[i].numRx++;
         neighbors_vars.neighbors[i].rssi=rssi;
         memcpy(&neighbors_vars.neighbors[i].asn,asnTimestamp,sizeof(asn_t));
         if (neighbors_vars.neighbors[i].stableNeighbor==FALSE) {
            if (neighbors_vars.neighbors[i].rssi>BADNEIGHBORMAXRSSI) {
               neighbors_vars.neighbors[i].switchStabilityCounter++;
               if (neighbors_vars.neighbors[i].switchStabilityCounter>=SWITCHSTABILITYTHRESHOLD) {
                  neighbors_vars.neighbors[i].switchStabilityCounter=0;
                  neighbors_vars.neighbors[i].stableNeighbor=TRUE;
               }
            } else {
               neighbors_vars.neighbors[i].switchStabilityCounter=0;
            }
         } else if (neighbors_vars.neighbors[i].stableNeighbor==TRUE) {
            if (neighbors_vars.neighbors[i].rssi<GOODNEIGHBORMINRSSI) {
               neighbors_vars.neighbors[i].switchStabilityCounter++;
               if (neighbors_vars.neighbors[i].switchStabilityCounter>=SWITCHSTABILITYTHRESHOLD) {
                  neighbors_vars.neighbors[i].switchStabilityCounter=0;
                   neighbors_vars.neighbors[i].stableNeighbor=FALSE;
               }
            } else {
               neighbors_vars.neighbors[i].switchStabilityCounter=0;
            }
         }
         return;
      }
      i++;   
   }
   // this is a new neighbor: register
   registerNewNeighbor(l2_src, rssi, asnTimestamp);
}

void neighbors_indicateTx(open_addr_t* dest,
                          uint8_t      numTxAttempts,
                          bool         was_finally_acked,
                          asn_t*       asnTimestamp) {
   uint8_t i=0;
   if (packetfunctions_isBroadcastMulticast(dest)==FALSE) {
      for (i=0;i<MAXNUMNEIGHBORS;i++) {
         if (isThisRowMatching(dest,i)) {
            if (neighbors_vars.neighbors[i].numTx==255) {
               neighbors_vars.neighbors[i].numTx/=2;
               neighbors_vars.neighbors[i].numTxACK/=2;
            }
            neighbors_vars.neighbors[i].numTx += numTxAttempts;
            if (was_finally_acked==TRUE) {
               neighbors_vars.neighbors[i].numTxACK++;
               memcpy(&neighbors_vars.neighbors[i].asn,asnTimestamp,sizeof(asn_t));
            }
            return;
         }
      }
   }
}

/**
\brief Find neighbor to which to send KA.

This function iterates through the neighbor table and identifies the neighbor
we need to send a KA to, if any. This neighbor satisfies the following
conditions:
- it is one of our preferred parents
- we haven't heard it for over 

\return A pointer to te neighbor's address, or NULL if no KA is needed.
*/
open_addr_t* neighbors_KaNeighbor() {
   uint8_t      i;
   uint16_t     timeSinceHeard;
   open_addr_t* addrPreferred;
   open_addr_t* addrOther;
   // initialize
   addrPreferred = NULL;
   addrOther     = NULL;
   // scan through the neighbor table, and populate addrPreferred and addrOther
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if (neighbors_vars.neighbors[i].used==1) {
         timeSinceHeard = ieee154e_asnDiff(&neighbors_vars.neighbors[i].asn);
         if (timeSinceHeard>KATIMEOUT) {
            // this neighbor needs to be KA'ed
            if (neighbors_vars.neighbors[i].parentPreference==MAXPREFERENCE) {
               // its a preferred parent
               addrPreferred = &(neighbors_vars.neighbors[i].addr_64b);
            } else {
               // its not a preferred parent
               // poipoi: don't KA to non-preferred parent
               //addrOther =     &(neighbors_vars.neighbors[i].addr_64b);
            }
         }
      }
   }
   // return the addr of the most urgent KA to send
   if        (addrPreferred!=NULL) {
      return addrPreferred;
   } else if (addrOther!=NULL) {
      return addrOther;
   } else {
      return NULL;
   }
}

// TODO: remove if not used
open_addr_t*  neighbors_getAddr(uint8_t neighboIdx) {
   return &neighbors_vars.neighbors[neighboIdx].addr_64b;
}

bool neighbors_isStableNeighbor(open_addr_t* address) {
   uint8_t i;
   open_addr_t temp_addr_64b;
   open_addr_t temp_prefix;
   switch (address->type) {
      case ADDR_128B:
         packetfunctions_ip128bToMac64b(address,&temp_prefix,&temp_addr_64b);
         break;
      default:
         openserial_printError(COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
                               (errorparameter_t)address->type,
                               (errorparameter_t)0);
         return FALSE;
   }
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if (isThisRowMatching(&temp_addr_64b,i) && neighbors_vars.neighbors[i].stableNeighbor==TRUE) {
         return TRUE;
      }
   }
   return FALSE;
}

 bool neighbors_isPreferredParent(open_addr_t* address) {
   uint8_t i;
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if (isThisRowMatching(address,i) && neighbors_vars.neighbors[i].parentPreference==MAXPREFERENCE) {
    	  ENABLE_INTERRUPTS();
         return TRUE;
      }
   }
   ENABLE_INTERRUPTS();
   return FALSE;
}

dagrank_t neighbors_getMyDAGrank() {
   return neighbors_vars.myDAGrank;
}

uint8_t neighbors_getNumNeighbors() {
   uint8_t i;
   uint8_t returnvalue=0;
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if (neighbors_vars.neighbors[i].used==TRUE) {
         returnvalue++;
      }
   }
   return returnvalue;
}

bool neighbors_getPreferredParent(open_addr_t* addressToWrite, uint8_t addr_type) {
   //following commented out section is equivalent to setting a default gw
   /*
      open_addr_t    nextHop;
      nextHop.type = ADDR_64B;
      nextHop.addr_64b[0]=0x00;
      nextHop.addr_64b[1]=0x00;
      nextHop.addr_64b[2]=0x00;
      nextHop.addr_64b[3]=0x00;
      nextHop.addr_64b[4]=0x00;
      nextHop.addr_64b[5]=0x00;
      nextHop.addr_64b[6]=0x00;
      nextHop.addr_64b[7]=0x01;
      memcpy(addressToWrite,&nextHop,sizeof(open_addr_t));
      */
   uint8_t i,posMinRank,usedNeighbours;
   dagrank_t minRank;
   bool preferred=FALSE;
   
   addressToWrite->type=ADDR_NONE;
   minRank=0xffff;//max rank
   usedNeighbours=0;
   
   posMinRank=MAXNUMNEIGHBORS+1;
   
   for (i=0; i<MAXNUMNEIGHBORS; i++) {
     if (neighbors_vars.neighbors[i].used==TRUE){
        if (neighbors_vars.neighbors[i].parentPreference==MAXPREFERENCE) {
         switch(addr_type) {
            case ADDR_64B:
               memcpy(addressToWrite,&(neighbors_vars.neighbors[i].addr_64b),sizeof(open_addr_t));
               addressToWrite->type=ADDR_64B;//set the type
               preferred=TRUE;
               break;
            default:
               openserial_printError(COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
                                     (errorparameter_t)addr_type,
                                     (errorparameter_t)1);
               preferred=FALSE;
               break;
              }
        }
        //get possible preferred parent in case it does not exist. Will be the one with lowest rank
        if( minRank > neighbors_vars.neighbors[i].DAGrank){
           minRank=neighbors_vars.neighbors[i].DAGrank;
           posMinRank=i;
        }
      usedNeighbours++;  //to see if there are neighbors
     }
   }
   if (preferred==FALSE && usedNeighbours > 0){
     //no preferred parent.. and at least one neighbour
      neighbors_vars.neighbors[posMinRank].parentPreference=MAXPREFERENCE;
      neighbors_vars.neighbors[posMinRank].stableNeighbor=TRUE;
      neighbors_vars.neighbors[posMinRank].switchStabilityCounter=0;
      //poipoi xv check if we need to update our rank..
      //neighbors_vars.myDAGrank=neighbors_vars.neighbors[posMinRank].DAGrank + 15;//tempLinkCost
      memcpy(addressToWrite,&(neighbors_vars.neighbors[posMinRank].addr_64b),sizeof(open_addr_t));
      addressToWrite->type=ADDR_64B;//set the type
      preferred=TRUE;         
   }
    return preferred;//this can be false if we don't have neighbours.        
}

bool debugPrint_neighbors() {
   debugNeighborEntry_t temp;
   neighbors_vars.debugRow=(neighbors_vars.debugRow+1)%MAXNUMNEIGHBORS;
   temp.row=neighbors_vars.debugRow;
   temp.neighborEntry=neighbors_vars.neighbors[neighbors_vars.debugRow];
   openserial_printStatus(STATUS_NEIGHBORS,(uint8_t*)&temp,sizeof(debugNeighborEntry_t));
   return TRUE;
}

/**
\brief Return a direct pointer to the neighbor table.

\note Modifying  this structure means you are modifying the neighbor table.
      Be careful when using the pointer; only read from the table.
NOBODY uses that. check and delete if not needed. poipoi xv
*/

//void neighbors_getAll(neighborRow_t* nlist){
//   nlist = &neighbors_vars.neighbors[0];
//}

/*returns a list of debug info
TODO, check that the number of bytes is not bigger than maxbytes. If so, retun error.*/
void neighbors_getNetDebugInfo(netDebugNeigborEntry_t *schlist,uint8_t maxbytes ){
  uint8_t j,size;
  size=0;
  
  for(j=0;j<MAXNUMNEIGHBORS;j++) {
     if(neighbors_vars.neighbors[j].used) {
       schlist[size].last_addr_byte = neighbors_vars.neighbors[j].addr_64b.addr_64b[7];//last byte of the address; poipoi could be [0]; endianness
       schlist[size].rssi = neighbors_vars.neighbors[j].rssi;
       schlist[size].parentPreference = neighbors_vars.neighbors[j].parentPreference;
       schlist[size].DAGrank = neighbors_vars.neighbors[j].DAGrank;
       memcpy(&schlist[size].asn,
              &neighbors_vars.neighbors[j].asn.bytes0and1,
              sizeof(neighbors_vars.neighbors[j].asn.bytes0and1));
       size++;//one more neighbour.
     }
   }  
}

//========================== Second Version of above function ===============================//
// To find the neighbors with lower DAGrank which could be a parent.
// if a neighbor at index(given) has a lower DAGrank than ref. rank givin then it returns 1 and copied the address in addressToWrite  !
//else it return zero
//index is used to get the neighbors one by one from an external function that passes in a loop up to the MAXNUMNEIGHBORS 
bool isNeighborsWithLowerDAGrank(dagrank_t RefRank, uint8_t index)
 {
   if(neighbors_vars.neighbors[index].used==TRUE && neighbors_vars.neighbors[index].DAGrank < RefRank)
   { 
      return TRUE;
   }
   else
     return FALSE;
   
 }

// Below function return the address for the neighbor at index and this function should be called after 
// the above function (isNeighborsWithLowerDAGrank) is being called.
void getNeighborsWithLowerDAGrank(uint8_t* addressToWrite,uint8_t addr_type, uint8_t index)
 { 
     switch(addr_type) {
            case ADDR_64B:
               memcpy(addressToWrite,&(neighbors_vars.neighbors[index].addr_64b.addr_64b),8);
               break;
            default:
               openserial_printError(COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
                                     (errorparameter_t)addr_type,
                                     (errorparameter_t)1);
               break; 
     }
 }



//=========================================================================================

// To find the neighbors with higher DAGrank which could be a children.
// if a neighbor at index(given) has a higher DAGrank than (ref. rank) givin then it returns 1 and copied the address in addressToWrite  !
//else it return zero
//index is used to get the neighbors one by one from an external function that passes in a loop up to the MAXNUMNEIGHBORS 
 bool getNeighborsWithHigherDAGrank(open_addr_t* addressToWrite,uint8_t addr_type, dagrank_t RefRank, uint8_t index)
 {
   if(neighbors_vars.neighbors[index].used==TRUE && neighbors_vars.neighbors[index].DAGrank > RefRank)
   {
     
     switch(addr_type) {
            case ADDR_64B:
               memcpy(addressToWrite,&(neighbors_vars.neighbors[index].addr_64b),sizeof(open_addr_t));
               addressToWrite->type=ADDR_64B;
               break;
            default:
               openserial_printError(COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
                                     (errorparameter_t)addr_type,
                                     (errorparameter_t)1);
               break; 
     }
      return TRUE;
   }
   else
     return FALSE;
   
 }


//=========================== private =========================================

void registerNewNeighbor(open_addr_t* address,
                         int8_t       rssi,
                         asn_t*       asnTimestamp) {
   uint8_t  i,j;
   bool     iHaveAPreferedParent;
   // filter errors
   if (address->type!=ADDR_64B) {
      openserial_printError(COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
                            (errorparameter_t)address->type,
                            (errorparameter_t)2);
      return;
   }
   // add this neighbor
   if (isNeighbor(address)==FALSE) {
      i=0;
      while(i<MAXNUMNEIGHBORS) {
         if (neighbors_vars.neighbors[i].used==FALSE) {
            // add this neighbor
            neighbors_vars.neighbors[i].used                   = TRUE;
            neighbors_vars.neighbors[i].parentPreference       = 0;
            //neighbors_vars.neighbors[i].stableNeighbor         = FALSE;
            // poipoi: all new neighbors are consider stable
            neighbors_vars.neighbors[i].stableNeighbor         = TRUE;
            neighbors_vars.neighbors[i].switchStabilityCounter = 0;
            memcpy(&neighbors_vars.neighbors[i].addr_64b,  address, sizeof(open_addr_t));
            neighbors_vars.neighbors[i].DAGrank                = 0xffff;
            neighbors_vars.neighbors[i].rssi                   = rssi;
            neighbors_vars.neighbors[i].numRx                  = 1;
            neighbors_vars.neighbors[i].numTx                  = 0;
            neighbors_vars.neighbors[i].numTxACK               = 0;
            memcpy(&neighbors_vars.neighbors[i].asn,asnTimestamp,sizeof(asn_t));
            // do I already have a preferred parent ?
            iHaveAPreferedParent = FALSE;
            for (j=0;j<MAXNUMNEIGHBORS;j++) {
               if (neighbors_vars.neighbors[j].parentPreference==MAXPREFERENCE) {
                  iHaveAPreferedParent = TRUE;
               }
            }
            // if I have none, and I'm not DAGroot, the new neighbor is my preferred
            if (iHaveAPreferedParent==FALSE && idmanager_getIsDAGroot()==FALSE) {      
               neighbors_vars.neighbors[i].parentPreference     = MAXPREFERENCE;
            }
            break;
         }
         i++;
      }
      if (i==MAXNUMNEIGHBORS) {
         openserial_printError(COMPONENT_NEIGHBORS,ERR_NEIGHBORS_FULL,
                               (errorparameter_t)MAXNUMNEIGHBORS,
                               (errorparameter_t)0);
         return;
      }
   }
   
}

bool isNeighbor(open_addr_t* neighbor) {
   uint8_t i=0;
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if (isThisRowMatching(neighbor,i)) {
         return TRUE;
      }
   }
   return FALSE;
}

void removeNeighbor(uint8_t neighborIndex) {
   neighbors_vars.neighbors[neighborIndex].used                      = FALSE;
   neighbors_vars.neighbors[neighborIndex].parentPreference          = 0;
   neighbors_vars.neighbors[neighborIndex].stableNeighbor            = FALSE;
   neighbors_vars.neighbors[neighborIndex].switchStabilityCounter    = 0;
   //neighbors_vars.neighbors[neighborIndex].addr_16b.type             = ADDR_NONE;//removed to save RAM
   neighbors_vars.neighbors[neighborIndex].addr_64b.type             = ADDR_NONE;
   //neighbors_vars.neighbors[neighborIndex].addr_128b.type            = ADDR_NONE;//removed to save RAM
   neighbors_vars.neighbors[neighborIndex].DAGrank                   = 0xffff;
   neighbors_vars.neighbors[neighborIndex].rssi                      = 0;
   neighbors_vars.neighbors[neighborIndex].numRx                     = 0;
   neighbors_vars.neighbors[neighborIndex].numTx                     = 0;
   neighbors_vars.neighbors[neighborIndex].numTxACK                  = 0;
   neighbors_vars.neighbors[neighborIndex].asn.bytes0and1            = 0;
   neighbors_vars.neighbors[neighborIndex].asn.bytes2and3            = 0;
   neighbors_vars.neighbors[neighborIndex].asn.byte4                 = 0;
}

bool isThisRowMatching(open_addr_t* address, uint8_t rowNumber) {
   switch (address->type) {
      case ADDR_64B:
         return neighbors_vars.neighbors[rowNumber].used &&
                packetfunctions_sameAddress(address,&neighbors_vars.neighbors[rowNumber].addr_64b);
      default:
         openserial_printError(COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
                               (errorparameter_t)address->type,
                               (errorparameter_t)3);
         return FALSE;
   }
}


//update neighbors_vars.myDAGrank and neighbor preference
void neighbors_updateMyDAGrankAndNeighborPreference() {
   uint8_t   i;
   uint8_t   temp_linkCost;
   uint32_t  temp_myTentativeDAGrank; //has to be 16bit so that the sum can be larger than 255 "CHANGED BY Ahmad to be 32 bits since the DAGrank is 16 bits now", so the sum will be >(0xFFFF)
   uint8_t   temp_preferredParentRow=0;
   bool      temp_preferredParentExists=FALSE;
   if ((idmanager_getIsDAGroot())==FALSE) {
      neighbors_vars.myDAGrank=0xffff;
      i=0;
      while(i<MAXNUMNEIGHBORS) {
         neighbors_vars.neighbors[i].parentPreference=0;
         //poipoi xv
         if (neighbors_vars.neighbors[i].used==TRUE /*&& neighbors_vars.neighbors[i].stableNeighbor==TRUE*/) {
            if (neighbors_vars.neighbors[i].numTxACK==0) {
               temp_linkCost=15; //TODO: evaluate using RSSI?
            } else {
               temp_linkCost=(uint8_t)((((float)neighbors_vars.neighbors[i].numTx)/((float)neighbors_vars.neighbors[i].numTxACK))*10.0);
            }
            temp_myTentativeDAGrank=neighbors_vars.neighbors[i].DAGrank+temp_linkCost;
           //poipoi xv -- avoiding dynamic routing...
            if (idmanager_getIsDAGroot()==FALSE && temp_myTentativeDAGrank<neighbors_vars.myDAGrank && temp_myTentativeDAGrank<0xffff) {
               
              neighbors_vars.myDAGrank=temp_myTentativeDAGrank;
              temp_preferredParentExists=TRUE;
              temp_preferredParentRow=i;
            }
               //the following is equivalent to manual routing 
#ifdef FORCE_MULTIHOP   
#ifdef GINA_FORCE_MULTIHOP   
            
               //   below to enforce the routing 
               switch ((idmanager_getMyID(ADDR_64B))->addr_64b[7]) {
               case 0x9B:
               if (neighbors_vars.neighbors[i].addr_64b.addr_64b[7]==0xDC) {
               neighbors_vars.myDAGrank=neighbors_vars.neighbors[i].DAGrank+temp_linkCost;
               temp_preferredParentExists=TRUE;
               temp_preferredParentRow=i;
               }
               break;
               case 0xDC:
               if (neighbors_vars.neighbors[i].addr_64b.addr_64b[7]==0xD8) {
               neighbors_vars.myDAGrank=neighbors_vars.neighbors[i].DAGrank+temp_linkCost;
               temp_preferredParentExists=TRUE;
               temp_preferredParentRow=i;
               }
               break;
               case 0xC9:
               if (neighbors_vars.neighbors[i].addr_64b.addr_64b[7]==0xED) {
               neighbors_vars.myDAGrank=neighbors_vars.neighbors[i].DAGrank+temp_linkCost;
               temp_preferredParentExists=TRUE;
               temp_preferredParentRow=i;
               }
               break;
               case 0xD8:
               if (neighbors_vars.neighbors[i].addr_64b.addr_64b[7]==0xC9) {
               neighbors_vars.myDAGrank=neighbors_vars.neighbors[i].DAGrank+temp_linkCost;
               temp_preferredParentExists=TRUE;
               temp_preferredParentRow=i;
               }
               break;
               default:
               break;
               }
          
#endif
#ifdef TELOSB_FORCE_MULTIHOP   
            
               //   below to enforce the routing 
               switch ((idmanager_getMyID(ADDR_64B))->addr_64b[7]) {
               case 0x51:
               if (neighbors_vars.neighbors[i].addr_64b.addr_64b[7]==0xB9) {
               neighbors_vars.myDAGrank=neighbors_vars.neighbors[i].DAGrank+temp_linkCost;
               temp_preferredParentExists=TRUE;
               temp_preferredParentRow=i;
               }
               break;
               case 0x41:
               if (neighbors_vars.neighbors[i].addr_64b.addr_64b[7]==0x51) {
               neighbors_vars.myDAGrank=neighbors_vars.neighbors[i].DAGrank+temp_linkCost;
               temp_preferredParentExists=TRUE;
               temp_preferredParentRow=i;
               }
               break;
               case 0x80:
               if (neighbors_vars.neighbors[i].addr_64b.addr_64b[7]==0x41) {
               neighbors_vars.myDAGrank=neighbors_vars.neighbors[i].DAGrank+temp_linkCost;
               temp_preferredParentExists=TRUE;
               temp_preferredParentRow=i;
               }
               break;
               case 0xE1:
               if (neighbors_vars.neighbors[i].addr_64b.addr_64b[7]==0x80) {
               neighbors_vars.myDAGrank=neighbors_vars.neighbors[i].DAGrank+temp_linkCost;
               temp_preferredParentExists=TRUE;
               temp_preferredParentRow=i;
               }
               break;
               default:
               break;
               }
          
#endif            
#endif
            
         }
         i++;
      }
      if (temp_preferredParentExists) {
         neighbors_vars.neighbors[temp_preferredParentRow].parentPreference=MAXPREFERENCE;
         neighbors_vars.neighbors[temp_preferredParentRow].stableNeighbor=TRUE;
         neighbors_vars.neighbors[temp_preferredParentRow].switchStabilityCounter = 0;
         
      }
   } else {
      neighbors_vars.myDAGrank=0;
   }
}
