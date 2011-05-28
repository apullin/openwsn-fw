#ifndef __IEEE802154E_H__
#define __IEEE802154E_H__

#include "openwsn.h"

/*----------------------------- IEEE802.15.4E ACK ---------------------------------------*/

typedef struct IEEE802154E_ACK_ht {
   uint8_t     dhrAckNack;
   uint16_t    timeCorrection;
} IEEE802154E_ACK_ht;

enum IEEE802154E_ACK_dhrAckNack_enums {
   IEEE154E_ACK_dhrAckNack_DEFAULT = 0x82,
};

/*----------------------------- IEEE802.15.4E ADV ---------------------------------------*/

typedef struct IEEE802154E_ADV_t {
   uint8_t     commandFrameId;
   uint32_t    timingInformation;   //needs to be 6 bytes long
   uint8_t     securityControlField;
   uint8_t     joinControl;
   uint8_t     timeslotHopping;
   uint8_t     channelPage;
   uint32_t    channelMap;
   uint8_t     numberSlotFrames;
   uint8_t     slotFrameID;
   uint16_t    slotFrameSize;
   uint8_t     numberLinks;
   uint32_t    linkInfo1;
   uint32_t    linkInfo2;
   uint8_t     DAGrank;
} IEEE802154E_ADV_t;

enum ieee154e_commandFrameId_enums {
   IEEE154E_ADV      = 0x0a,
   IEEE154E_JOIN     = 0x0b,
   IEEE154E_ACTIVATE = 0x0c,
};

enum ieee154e_ADV_defaults_enums {
   IEEE154E_ADV_SEC_DEFAULT           = 0x00,
   IEEE154E_ADV_JOINCONTROL_DEFAULT   = 0x00,
   IEEE154E_ADV_HOPPING_DEFAULT       = 0x00,
   IEEE154E_ADV_CHANNELPAGE_DEFAULT   = 0x04,
   IEEE154E_ADV_CHANNELMAP_DEFAULT    = 0x07FFF800,
   IEEE154E_ADV_NUMSLOTFRAMES_DEFAULT = 0x01,
   IEEE154E_ADV_SLOTFRAMEID_DEFAULT   = 0x00,
   IEEE154E_ADV_SLOTFRAMESIZE_DEFAULT = 0x001F,
   IEEE154E_ADV_NUMLINKS_DEFAULT      = 0x02,
   IEEE154E_ADV_LINKINFO1_DEFAULT     = 0x00000002,
   IEEE154E_ADV_LINKINFO2_DEFAULT     = 0x00010001,
};

/*----------------------------- SLOT STRUCTURES ---------------------------------------*/
enum {
   //synchronizing
   S_SYNCHRONIZING       =  0,
   //transmitter
   S_TX_TXDATAPREPARE    =  1,
   S_TX_TXDATAREADY      =  2,
   S_TX_TXDATA           =  3,
   S_TX_RXACKPREPARE     =  4,
   S_TX_RXACKREADY       =  5,
   S_TX_RXACK            =  6,
   //receiver
   S_RX_RXDATAPREPARE    =  7,
   S_RX_RXDATAREADY      =  8,
   S_RX_RXDATA           =  9,
   S_RX_TXACKPREPARE     = 10,
   S_RX_TXACKREADY       = 11,
   S_RX_TXACK            = 12,
   //cooldown
   S_SLEEP               = 13,
};


//added fotr TSCH

enum {
   FRAME_BASED_RESYNC = TRUE,
   ACK_BASED_RESYNC = FALSE,
};

enum {
   WAS_ACKED = TRUE,
   WAS_NOT_ACKED = FALSE,
};

//timer wait times (in 1/32768 seconds)
enum {
   PERIODICTIMERPERIOD   =    326,// 10 ms  //uses TIMER_MAC_PERIODIC
   MINBACKOFF            =    65,// 2ms     //uses TIMER_MAC_BACKOFF 
                                            //will add EXTRA_WAIT_TIME later if receiving 
   GUARDTIME             =    130,//4 ms    //uses TIMER_MAC_WATCHDOG
   ACK_WAIT_TIME         =    195,// 6ms    //uses TIMER_MAC_BACKOFF 
                                            //will add EXTRA_WAIT_TIME later if receiving
   EXTRA_WAIT_TIME       =    32, //1 ms    //this is used to add 1ms to the receiver for overlap
};


void    mac_init();
error_t mac_send(OpenQueueEntry_t* msg);
void    mac_sendDone(OpenQueueEntry_t* pkt, error_t error);
void radio_packet_received(OpenQueueEntry_t* msg);
bool    mac_debugPrint();
void radio_prepare_send_done();


#endif
