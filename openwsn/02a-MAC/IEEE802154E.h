/**
\brief IEEE802.15.4e TSCH

\author Branko Kerkez   <bkerkez@berkeley.edu>, March 2011
\author Fabien Chraim   <chraim@eecs.berkeley.edu>, June 2011
\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
 */

#ifndef __IEEE802154E_H
#define __IEEE802154E_H

#include "openwsn.h"

//=========================== define ==========================================

// this is the channel the mote will listen on to synchronize
#define SYNCHRONIZING_CHANNEL 26
#define TXRETRIES              3
#define SYNCTIMEOUT          200 // @10ms per slot -> 2 second
//1=-25dBm, 31=0dBm (max value)
#define TX_POWER              31

// the different states of the IEEE802.15.4e state machine
enum ieee154e_state_enum {
   S_SLEEP                   = 0x00,   // ready for next slot
   // synchronizing
   S_SYNCHRONIZING           = 0x01,   // synchronizing to the network
   // TX
   S_TXDATAOFFSET            = 0x02,   // waiting to prepare for Tx data
   S_TXDATAPREPARE           = 0x03,   // preparing for Tx data
   S_TXDATAREADY             = 0x04,   // ready to Tx data, waiting for 'go'
   S_TXDATADELAY             = 0x05,   // 'go' signal given, waiting for SFD Tx data
   S_TXDATA                  = 0x06,   // Tx data SFD received, sending bytes
   S_RXACKOFFSET             = 0x07,   // Tx data done, waiting to prepare for Rx ACK
   S_RXACKPREPARE            = 0x08,   // preparing for Rx ACK
   S_RXACKREADY              = 0x09,   // ready to Rx ACK, waiting for 'go'
   S_RXACKLISTEN             = 0x0a,   // idle listening for ACK
   S_RXACK                   = 0x0b,   // Rx ACK SFD received, receiving bytes
   S_TXPROC                  = 0x0c,   // processing sent data
   // RX
   S_RXDATAOFFSET            = 0x0d,   // waiting to prepare for Rx data
   S_RXDATAPREPARE           = 0x0e,   // preparing for Rx data
   S_RXDATAREADY             = 0x0f,   // ready to Rx data, waiting for 'go'
   S_RXDATALISTEN            = 0x10,   // idle listening for data
   S_RXDATA                  = 0x11,   // data SFD received, receiving more bytes
   S_TXACKOFFSET             = 0x12,   // waiting to prepare for Tx ACK
   S_TXACKPREPARE            = 0x13,   // preparing for Tx ACK
   S_TXACKREADY              = 0x14,   // Tx ACK ready, waiting for 'go'
   S_TXACKDELAY              = 0x15,   // 'go' signal given, waiting for SFD Tx ACK
   S_TXACK                   = 0x16,   // Tx ACK SFD received, sending bytes
   S_RXPROC                  = 0x17,   // processing received data
};

// Atomic durations
// expressed in 32kHz ticks:
//    - ticks = duration_in_seconds * 32768
//    - duration_in_seconds = ticks / 32768
enum ieee154e_atomicdurations_enum{
   // time-slot related
   TsTxOffset                =  69,    //  2120us
   TsLongGT                  =  33,    //  1000us
   TsTxAckDelay              =  33,    //  1000us
   TsShortGT                 =  16,    //   500us
   TsSlotDuration            = 328,    // 10000us
   // execution speed related
   maxTxDataPrepare          =  33,    //  1000us (TBC)
   maxRxAckPrepare           =  10,    //   (TBC)
   maxRxDataPrepare          =  13,    //   400us (TBC)
   maxTxAckPrepare           =  17,    //   (TBC)
   // radio speed related
   delayTx                   =   0,    //     0us (TBC)
   delayRx                   =   0,    //     0us (TBC)
   // radio watchdog
   wdRadioTx                 =  33,    //  1000us
   wdDataDuration            = 164,    //  5000us
   wdAckDuration             =  98,    //  3000us
};

// FSM timer durations (combinations of atomic durations)
// TX
#define DURATION_tt1 ieee154e_vars.capturedTime+TsTxOffset-delayRx-maxRxDataPrepare
#define DURATION_tt2 ieee154e_vars.capturedTime+TsTxOffset-delayTx
#define DURATION_tt3 ieee154e_vars.capturedTime+TsTxOffset-delayTx+wdRadioTx
#define DURATION_tt4 ieee154e_vars.capturedTime+wdDataDuration
#define DURATION_tt5 ieee154e_vars.capturedTime+TsTxAckDelay-TsShortGT-delayRx-maxRxAckPrepare
#define DURATION_tt6 ieee154e_vars.capturedTime+TsTxAckDelay-TsShortGT-delayRx
#define DURATION_tt7 ieee154e_vars.capturedTime+TsTxAckDelay+TsShortGT
#define DURATION_tt8 ieee154e_vars.capturedTime+wdAckDuration
// RX
#define DURATION_rt1 ieee154e_vars.capturedTime+TsTxOffset-TsLongGT-delayRx-maxRxDataPrepare
#define DURATION_rt2 ieee154e_vars.capturedTime+TsTxOffset-TsLongGT-delayRx
#define DURATION_rt3 ieee154e_vars.capturedTime+TsTxOffset+TsLongGT
#define DURATION_rt4 ieee154e_vars.capturedTime+wdDataDuration
#define DURATION_rt5 ieee154e_vars.capturedTime+TsTxAckDelay-delayTx-maxTxAckPrepare
#define DURATION_rt6 ieee154e_vars.capturedTime+TsTxAckDelay-delayTx
#define DURATION_rt7 ieee154e_vars.capturedTime+TsTxAckDelay-delayTx+wdRadioTx
#define DURATION_rt8 ieee154e_vars.capturedTime+wdAckDuration

//=========================== typedef =========================================

//IEEE802.15.4E acknowledgement (ACK)
typedef struct {
   uint16_t    timeCorrection;
} IEEE802154E_ACK_ht;

//IEEE802.15.4E advertisement (ADV)
typedef struct {
   uint16_t   asn;
} IEEE802154E_ADV_t;

//=========================== variables =======================================

//=========================== prototypes ======================================


// from upper layer
void    mac_init();
error_t mac_send(OpenQueueEntry_t* msg);

// events
void    ieee154e_startOfFrame(uint16_t capturedTime);
void    ieee154e_endOfFrame(uint16_t capturedTime);

// misc
bool    mac_debugPrint();

#endif