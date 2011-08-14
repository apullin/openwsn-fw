/**
\brief Implementation of neighbors

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
*/

#include "openwsn.h"
#include "neighbors.h"
#include "openqueue.h"
#include "packetfunctions.h"
#include "idmanager.h"
#include "openserial.h"
#include "IEEE802154E.h"

//=========================== variables =======================================

typedef struct {
   neighborEntry_t  neighbors[MAXNUMNEIGHBORS];
   dagrank_t        myDAGrank;
   uint8_t          debugRow;
} neighbors_vars_t;

neighbors_vars_t neighbors_vars;

//=========================== prototypes ======================================

void registerNewNeighbor(open_addr_t* neighborID,dagrank_t hisDAGrank);
bool isNeighbor(open_addr_t* neighbor);
void removeNeighbor(uint8_t neighborIndex);
bool isThisRowMatching(open_addr_t* address, uint8_t rowNumber);

//=========================== public ==========================================

void neighbors_init() {
   uint8_t i;
   for (i=0;i<MAXNUMNEIGHBORS;i++){
      neighbors_vars.neighbors[i].used=FALSE;
   }
   if (idmanager_getIsDAGroot()==TRUE) {
      neighbors_vars.myDAGrank=0;
   } else {
      neighbors_vars.myDAGrank=255;
   }
}

void neighbors_receiveDIO(OpenQueueEntry_t* msg) {
   uint8_t i;
   uint8_t temp_linkCost;
   msg->owner = COMPONENT_NEIGHBORS;
   if (isNeighbor(&(msg->l2_nextORpreviousHop))==TRUE) {
      for (i=0;i<MAXNUMNEIGHBORS;i++) {
         if (isThisRowMatching(&(msg->l2_nextORpreviousHop),i)) {
            //memcpy(&(neighbors[i].addr_128b),&(msg->l3_destinationORsource),sizeof(open_addr_t));//removed to save RAM
            neighbors_vars.neighbors[i].DAGrank = *((uint8_t*)(msg->payload));
            //poipoi forces single hop
            if (neighbors_vars.neighbors[i].DAGrank==0x00) {
               neighbors_vars.neighbors[i].parentPreference=MAXPREFERENCE;
               if (neighbors_vars.neighbors[i].numTxACK==0) {
                  temp_linkCost=15; //TODO: evaluate using RSSI?
               } else {
                  temp_linkCost=(uint8_t)((((float)neighbors_vars.neighbors[i].numTx)/((float)neighbors_vars.neighbors[i].numTxACK))*10.0);
               }
               neighbors_vars.myDAGrank=neighbors_vars.neighbors[i].DAGrank+temp_linkCost;
            }
            break;
         }
      }
   } else {
      registerNewNeighbor(&(msg->l2_nextORpreviousHop),*((uint8_t*)(msg->payload)) );
   }
   //neighbors_updateMyDAGrankAndNeighborPreference(); poipoi forces single hop
}

void neighbors_indicateRx(open_addr_t* l2_src,uint16_t rssi) {
   uint8_t i=0;
   while (i<MAXNUMNEIGHBORS) {
      if (isThisRowMatching(l2_src,i)) {
         neighbors_vars.neighbors[i].numRx++;
         neighbors_vars.neighbors[i].linkQuality=rssi;
         neighbors_vars.neighbors[i].timestamp=0;//poipoi implement timing
         if (neighbors_vars.neighbors[i].stableNeighbor==FALSE) {
            if (neighbors_vars.neighbors[i].linkQuality>BADNEIGHBORMAXPOWER || neighbors_vars.neighbors[i].linkQuality<129) {
               neighbors_vars.neighbors[i].switchStabilityCounter++;
               if (neighbors_vars.neighbors[i].switchStabilityCounter>=SWITCHSTABILITYTHRESHOLD) {
                  neighbors_vars.neighbors[i].switchStabilityCounter=0;
                  neighbors_vars.neighbors[i].stableNeighbor=TRUE;
               }
            } else {
               neighbors_vars.neighbors[i].switchStabilityCounter=0;
            }
         } else if (neighbors_vars.neighbors[i].stableNeighbor==TRUE) {
            if (neighbors_vars.neighbors[i].linkQuality<GOODNEIGHBORMINPOWER && neighbors_vars.neighbors[i].linkQuality>128) {
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
}

void neighbors_indicateTx(open_addr_t* l2_dest, bool was_acked) {
   uint8_t i=0;
   if (packetfunctions_isBroadcastMulticast(l2_dest)==FALSE) {
      for (i=0;i<MAXNUMNEIGHBORS;i++) {
         if (isThisRowMatching(l2_dest,i)) {
            if (neighbors_vars.neighbors[i].numTx==255) {
               neighbors_vars.neighbors[i].numTx/=2;
               neighbors_vars.neighbors[i].numTxACK/=2;
            }
            neighbors_vars.neighbors[i].numTx++;
            if (was_acked==TRUE) {
               neighbors_vars.neighbors[i].numTxACK++;
               neighbors_vars.neighbors[i].timestamp=0;//poipoi implement timing
            }
            return;
         }
      }
   }
}

bool neighbors_isStableNeighbor(open_addr_t* address) {
   uint8_t i=0;
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
void neighbors_getPreferredParent(open_addr_t* addressToWrite, uint8_t addr_type) {
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
   uint8_t i;
   addressToWrite->type=ADDR_NONE;
   for (i=0; i<MAXNUMNEIGHBORS; i++) {
      if (neighbors_vars.neighbors[i].used==TRUE && neighbors_vars.neighbors[i].parentPreference==MAXPREFERENCE) {
         switch(addr_type) {
            /*case ADDR_16B:
              memcpy(addressToWrite,&(neighbors[i].addr_16b),sizeof(open_addr_t));
              break;*///removed to save RAM
            case ADDR_64B:
               memcpy(addressToWrite,&(neighbors_vars.neighbors[i].addr_64b),sizeof(open_addr_t));//poipoi I only really use this one
               break;
               /*case ADDR_128B:
                 memcpy(addressToWrite,&(neighbors[i].addr_128b),sizeof(open_addr_t));
                 break;*///removed to save RAM
            default:
               openserial_printError(COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
                     (errorparameter_t)addr_type,
                     (errorparameter_t)0);
               break;
         }
         return;
      }
   }
}

bool neighbors_debugPrint() {

   debugNeighborEntry_t temp;
   uint8_t num_iterations;
   num_iterations = 0;
   do {
      neighbors_vars.debugRow=(neighbors_vars.debugRow+1)%MAXNUMNEIGHBORS;
      num_iterations++;
   } while (neighbors_vars.neighbors[neighbors_vars.debugRow].used==0 && num_iterations<MAXNUMNEIGHBORS+1);
   if (num_iterations<MAXNUMNEIGHBORS+1) {
      temp.row=neighbors_vars.debugRow;
      temp.neighborEntry=neighbors_vars.neighbors[neighbors_vars.debugRow];
      openserial_printStatus(STATUS_NEIGHBORS_NEIGHBORS,(uint8_t*)&temp,sizeof(debugNeighborEntry_t));
      return TRUE;
   }
   return FALSE;

   /*debugNeighborEntry_t temp;
     neighbors_vars.debugRow=(neighbors_vars.debugRow+1)%MAXNUMNEIGHBORS;
     temp.row=neighbors_vars.debugRow;
     temp.neighborEntry=neighbors[neighbors_vars.debugRow];
     openserial_printStatus(STATUS_NEIGHBORS_NEIGHBORS,(uint8_t*)&temp,sizeof(debugNeighborEntry_t));
     return TRUE;*/
}

//=========================== private =========================================

void registerNewNeighbor(open_addr_t* address,dagrank_t hisDAGrank) {
   /*open_addr_t temp_prefix;
     open_addr_t temp_addr16b;
     open_addr_t temp_addr64b;
     open_addr_t temp_addr128b;*///removed to save RAM
   uint8_t  i;
   if (address->type!=ADDR_16B && address->type!=ADDR_64B && address->type!=ADDR_128B) {
      openserial_printError(COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
            (errorparameter_t)address->type,
            (errorparameter_t)1);
      return;
   }
   if (isNeighbor(address)==FALSE) {
      i=0;
      while(i<MAXNUMNEIGHBORS) {
         if (neighbors_vars.neighbors[i].used==FALSE) {
            neighbors_vars.neighbors[i].used                   = TRUE;
            neighbors_vars.neighbors[i].parentPreference       = 0;
            neighbors_vars.neighbors[i].stableNeighbor         = TRUE;//poipoipoi speed up for workshop
            neighbors_vars.neighbors[i].switchStabilityCounter = 0;
            //neighbors[i].addr_16b.type          = ADDR_NONE;//removed to save RAM
            neighbors_vars.neighbors[i].addr_64b.type          = ADDR_NONE;
            //neighbors[i].addr_128b.type         = ADDR_NONE;//removed to save RAM
            switch (address->type) {
               /*case ADDR_16B:
                 packetfunctions_mac16bToMac64b(address,&temp_addr64b);
                 packetfunctions_mac64bToIp128b(
                 idmanager_getMyID(ADDR_PREFIX),
                 &temp_addr64b,
                 &temp_addr128b);
                 memcpy(&neighbors[i].addr_16b,  address,        sizeof(open_addr_t));
                 memcpy(&neighbors[i].addr_64b,  &temp_addr64b,  sizeof(open_addr_t));
                 memcpy(&neighbors[i].addr_128b, &temp_addr128b, sizeof(open_addr_t));
                 break;*///removed to save RAM
               case ADDR_64B:
                  /*packetfunctions_mac64bToMac16b(address,&temp_addr16b);
                    packetfunctions_mac64bToIp128b(
                    idmanager_getMyID(ADDR_PREFIX),
                    address,
                    &temp_addr128b);
                    memcpy(&neighbors[i].addr_16b,  &temp_addr16b,  sizeof(open_addr_t));*///removed to save RAM
                  memcpy(&neighbors_vars.neighbors[i].addr_64b,  address,        sizeof(open_addr_t));
                  //memcpy(&neighbors[i].addr_128b, &temp_addr128b, sizeof(open_addr_t));//removed to save RAM
                  break;
                  /*case ADDR_128B:
                    packetfunctions_ip128bToMac64b(
                    address,
                    &temp_prefix,
                    &temp_addr64b);
                    packetfunctions_mac64bToMac16b(&temp_addr64b,&temp_addr16b);
                    memcpy(&neighbors[i].addr_16b,  &temp_addr16b,  sizeof(open_addr_t));
                    memcpy(&neighbors[i].addr_64b,  &temp_addr64b,  sizeof(open_addr_t));
                    memcpy(&neighbors[i].addr_128b, address,        sizeof(open_addr_t));
                    break;*///removed to save RAM
            }
            neighbors_vars.neighbors[i].DAGrank                = hisDAGrank;
            neighbors_vars.neighbors[i].linkQuality            = 0;
            neighbors_vars.neighbors[i].numRx                  = 1;
            neighbors_vars.neighbors[i].numTx                  = 0;
            neighbors_vars.neighbors[i].numTxACK               = 0;
            neighbors_vars.neighbors[i].timestamp              = 0;//poipoi implement timing
            break;
         }
         i++;
      }
      if (i==MAXNUMNEIGHBORS) {
         openserial_printError(COMPONENT_NEIGHBORS,ERR_NEIGHBORS_FULL,MAXNUMNEIGHBORS,0);
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
   neighbors_vars.neighbors[neighborIndex].DAGrank                   = 255;
   neighbors_vars.neighbors[neighborIndex].linkQuality               = 0;
   neighbors_vars.neighbors[neighborIndex].numRx                     = 0;
   neighbors_vars.neighbors[neighborIndex].numTx                     = 0;
   neighbors_vars.neighbors[neighborIndex].numTxACK                  = 0;
   neighbors_vars.neighbors[neighborIndex].timestamp                 = 0;
}

bool isThisRowMatching(open_addr_t* address, uint8_t rowNumber) {
   switch (address->type) {
      /*case ADDR_16B:
        return neighbors[rowNumber].used &&
        packetfunctions_sameAddress(address,&neighbors[rowNumber].addr_16b);*///removed to save RAM
      case ADDR_64B:
         return neighbors_vars.neighbors[rowNumber].used &&
            packetfunctions_sameAddress(address,&neighbors_vars.neighbors[rowNumber].addr_64b);
         /*case ADDR_128B:
           return neighbors[rowNumber].used &&
           packetfunctions_sameAddress(address,&neighbors[rowNumber].addr_128b);*///removed to save RAM
      default:
         openserial_printError(COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
               (errorparameter_t)address->type,
               (errorparameter_t)2);
         return FALSE;
   }
}


//update neighbors_vars.myDAGrank and neighbor preference
void neighbors_updateMyDAGrankAndNeighborPreference() {
   uint8_t   i;
   uint8_t   temp_linkCost;
   uint16_t  temp_myTentativeDAGrank; //has to be 16bit, so that the sum can be larger than 255
   uint8_t   temp_preferredParentRow=0;
   bool      temp_preferredParentExists=FALSE;
   if ((idmanager_getIsDAGroot())==FALSE) {
      neighbors_vars.myDAGrank=255;
      i=0;
      while(i<MAXNUMNEIGHBORS) {
         neighbors_vars.neighbors[i].parentPreference=0;
         if (neighbors_vars.neighbors[i].used==TRUE && neighbors_vars.neighbors[i].stableNeighbor==TRUE) {
            if (neighbors_vars.neighbors[i].numTxACK==0) {
               temp_linkCost=15; //TODO: evaluate using RSSI?
            } else {
               temp_linkCost=(uint8_t)((((float)neighbors_vars.neighbors[i].numTx)/((float)neighbors_vars.neighbors[i].numTxACK))*10.0);
            }
            temp_myTentativeDAGrank=neighbors_vars.neighbors[i].DAGrank+temp_linkCost;
            if (temp_myTentativeDAGrank<neighbors_vars.myDAGrank && temp_myTentativeDAGrank<255) {
               neighbors_vars.myDAGrank=temp_myTentativeDAGrank;
               temp_preferredParentExists=TRUE;
               temp_preferredParentRow=i;
            }
            //the following is equivalent to manual routing
            /*
               switch ((idmanager_getMyID(ADDR_16B))->addr_16b[1]) {
               case 0x03:
               if (neighbors[i].addr_16b.addr_16b[1]==0x07) {
               neighbors_vars.myDAGrank=neighbors[i].DAGrank+temp_linkCost;
               temp_preferredParentExists=TRUE;
               temp_preferredParentRow=i;
               }
               break;
               case 0x07:
               if (neighbors[i].addr_16b.addr_16b[1]==0x01) {
               neighbors_vars.myDAGrank=neighbors[i].DAGrank+temp_linkCost;
               temp_preferredParentExists=TRUE;
               temp_preferredParentRow=i;
               }
               break;
               default:
               break;
               }
               */
         }
         i++;
      }
      if (temp_preferredParentExists) {
         neighbors_vars.neighbors[temp_preferredParentRow].parentPreference=MAXPREFERENCE;
      }
   } else {
      neighbors_vars.myDAGrank=0;
   }
}
