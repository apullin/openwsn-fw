#ifndef __NEIGHBORS_H
#define __NEIGHBORS_H

/**
\addtogroup MAChigh
\{
\addtogroup Neighbors
\{
*/

//=========================== define ==========================================

#define MAXNUMNEIGHBORS            10
#define MAXPREFERENCE               2
//-70dBm in 8-bit 2's compl. (-70 -> -25 -> 26 -> 0001 1010 -> 1110 0101-> 229)
#define BADNEIGHBORMAXPOWER       229
//-80dBm in 8-bit 2's compl. (-80 -> -35 -> 36 -> 219)
#define GOODNEIGHBORMINPOWER      219
#define SWITCHSTABILITYTHRESHOLD    3

//=========================== typedef =========================================

typedef struct {
   bool             used;
   uint8_t          parentPreference;
   bool             stableNeighbor;
   uint8_t          switchStabilityCounter;
   open_addr_t      addr_64b;
   dagrank_t        DAGrank;
   uint8_t          linkQuality;
   uint8_t          numRx;
   uint8_t          numTx;
   uint8_t          numTxACK;
   timervalue_t     timestamp;
} neighborEntry_t;

typedef struct {
   slotOffset_t    row;
   neighborEntry_t neighborEntry;
} debugNeighborEntry_t;

//=========================== variables =======================================

//=========================== prototypes ======================================

void      neighbors_init();
void      neighbors_receiveDIO(OpenQueueEntry_t* msg);
void      neighbors_updateMyDAGrankAndNeighborPreference();
void      neighbors_indicateRx(open_addr_t* l2_src,
                               uint16_t     rssi,
                               asn_t        asnTimestamp);
void      neighbors_indicateTx(open_addr_t* dest,
                               uint8_t      numTxAttempts,
                               bool         was_finally_acked,
                               asn_t        asnTimestamp);
bool      neighbors_isStableNeighbor(open_addr_t* address);
dagrank_t neighbors_getMyDAGrank();
uint8_t   neighbors_getNumNeighbors();
void      neighbors_getPreferredParent(open_addr_t* addressToWrite,
                                       uint8_t addr_type);
bool      debugPrint_neighbors();

/**
\}
\}
*/

#endif
