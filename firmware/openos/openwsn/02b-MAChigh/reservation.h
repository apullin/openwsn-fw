/**
\addtogroup MAChigh
\{
\addtogroup Reservation
\{
*/

#include "openwsn.h"
#include "openserial.h"
#include "IEEE802154.h"
#include "IEEE802154E.h"
#include "schedule.h"
#include "neighbors.h"
#include "iphc.h"
#include "openqueue.h"


//=========================== define ==========================================

#define MAXRXCELL           8     // the maximum number of Rx cells for one node
#define RES_PAYLOAD_LENGTH  7     //1B: =0, distinguish from Iphc frame, 1B: command code, 4B: parameter, 1B: NumOfCell

#define RES_CELL_REQUEST    0x21  //reservation command code
#define REMOVE_CELL_REQUEST 0x22  //reservation command code
#define RES_CELL_RESPONSE   0x23  //reservation command code

#define SUPERFRAMELENGTH    16    //length of superframe
#define NUMOFCHANNEL        8     //number of avariable channel

#define SUCCESS             1
#define FAILURE             0

#define RESERVATION_TIMEOUT         30000  //in ms

typedef enum {
    S_IDLE                      = 0x00,   // ready for next event
    // RESCELLREQUEST
    S_TXBUSYCHECK               = 0x01,   // waiting for Busy Check result
    S_SENDOUTRESCELLREQUEST     = 0x02,   // waiting for SendDone confirmation
    S_WAITFORRESPONSE           = 0x03,   // waiting for response from the neighbor
    S_SENDOUTRESCELLRESPONSE    = 0x04,   // waiting for SendDone confirmation
    S_HIDENCHECK                = 0x05,   // waiting for Hiden Terminal Check result
    // REMOVECELLREQUEST
    S_SENDOUTREMOVECELLREQUEST      = 0x06    // waiting for SendDone confirmation
} reservation_state_t;

typedef void (*reservation_granted_cbt)(void);

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================
void    reservation_init();


//called by res.c
void    reservation_GetSeedAndBitMap(uint8_t* Seed, uint8_t* BitMap);
void    reservation_CommandSendDone(OpenQueueEntry_t* msg);
void    reservation_RecordSeedAndBitMap(OpenQueueEntry_t* msg);
void    reservation_CommandReceive(OpenQueueEntry_t* msg);
//for testing
void    isr_reservation_button();
//called by IEEE802154E.c
void    reservation_IndicateBusyCheck(); 
//called by upper layer
void reservation_LinkRequest(open_addr_t* NeighborAddr, uint8_t NumOfCell);
void reservation_RemoveLinkRequest(open_addr_t* NeighborAddr, uint8_t RequiredNumOfCell);
void reservation_setcb(reservation_granted_cbt reservationGrantedCb,reservation_granted_cbt reservationFailedCb);
