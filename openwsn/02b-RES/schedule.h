#ifndef __SCHEDULE_H
#define __SCHEDULE_H

/**
\addtogroup MAChigh
\{
\addtogroup Schedule
\{
*/

#include "openwsn.h"

//=========================== define ==========================================
#define SCHEDULELENGTH  9

enum {
   CELLTYPE_OFF         = 0,
   CELLTYPE_ADV         = 1,
   CELLTYPE_TX          = 2,
   CELLTYPE_RX          = 3,
   CELLTYPE_SERIALRX    = 4
};

//=========================== typedef =========================================

typedef uint8_t    cellType_t;
typedef uint8_t    channelOffset_t;

typedef struct {
   uint8_t         type;
   uint8_t         channelOffset;
   open_addr_t     neighbor;
   uint8_t         numUsed;
   uint8_t         numTxACK;
   timervalue_t    timestamp;
} scheduleRow_t;

typedef struct {
   uint8_t         row;
   scheduleRow_t   cellUsage;
} debugScheduleRow_t;

//=========================== variables =======================================

//=========================== prototypes ======================================

          void            schedule_init();
          bool            debugPrint_schedule();
__monitor cellType_t      schedule_getType(asn_t asn_param);
__monitor channelOffset_t schedule_getChannelOffset(asn_t asn_param);
__monitor void            schedule_getNeighbor(asn_t asn_param, open_addr_t* addrToWrite);

/**
\}
\}
*/
          
#endif