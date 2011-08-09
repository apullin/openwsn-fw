/*
 * The component which manages the pool of packet buffers.
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef OPENQUEUE_H
#define OPENQUEUE_H

#include "openwsn.h"
#include "IEEE802154.h"

typedef struct debugOpenQueueEntry_t {
   uint8_t  creator;
   uint8_t  owner;
} debugOpenQueueEntry_t;

void               openqueue_init();
OpenQueueEntry_t*  openqueue_getFreePacketBuffer();
error_t            openqueue_freePacketBuffer(OpenQueueEntry_t* pkt);
OpenQueueEntry_t*  openqueue_inQueue(bool isADV);
error_t            openqueue_removeAllPacketsToNeighbor(open_addr_t* neighbor);
void               openqueue_removeAllOwnedBy(uint8_t owner);
void               openqueue_reset_entry(OpenQueueEntry_t* entry);
bool               openqueue_debugPrint();

#endif
