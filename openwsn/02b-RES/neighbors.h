/*
 * Implementation of neighbors
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef __NEIGHBORS_H
#define __NEIGHBORS_H

typedef struct neighborEntry_t {
   bool             used;
   uint8_t          parentPreference;
   bool             stableNeighbor;
   uint8_t          switchStabilityCounter;
   //open_addr_t      addr_16b;        //removed to save RAM
   open_addr_t      addr_64b;
   //open_addr_t      addr_128b;       //removed to save RAM
   dagrank_t        DAGrank;
   uint8_t          linkQuality;
   uint8_t          numRx;
   uint8_t          numTx;
   uint8_t          numTxACK;
   timervalue_t     timestamp;
} neighborEntry_t;

typedef struct debugNeighborEntry_t {
   slotOffset_t    row;
   neighborEntry_t neighborEntry;
} debugNeighborEntry_t;

void      neighbors_init();
void      neighbors_receiveDIO(OpenQueueEntry_t* msg);
void      neighbors_updateMyDAGrankAndNeighborPreference();
void      neighbors_indicateRx(open_addr_t* l2_src,uint16_t rssi);
void      neighbors_indicateTx(open_addr_t* dest, bool was_acked);
bool      neighbors_isStableNeighbor(open_addr_t* address);
dagrank_t neighbors_getMyDAGrank();
uint8_t   neighbors_getNumNeighbors();
void      neighbors_getPreferredParent(open_addr_t* addressToWrite, uint8_t addr_type);
bool      neighbors_debugPrint();

#endif
