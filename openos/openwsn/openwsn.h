/*
 * General OpenWSN definitions
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 * Ankur Mehta <mehtank@eecs.berkeley.edu>, September 2010
 */

#ifndef __OPENWSN_H
#define __OPENWSN_H

//general
#include "msp430x26x.h"
#include "stdint.h"                              // needed for uin8_t, uint16_t
#include <string.h>                              // needed for memcpy and memcmp

#define bool uint8_t

#define DEBUG_PIN_FRAME_INIT()    P4DIR |=  0x20 // P4.5
#define DEBUG_PIN_FRAME_TOGGLE()  P4OUT ^=  0x20
#define DEBUG_PIN_FRAME_CLR()     P4OUT &= ~0x20
#define DEBUG_PIN_FRAME_SET()     P4OUT |=  0x20

#define DEBUG_PIN_SLOT_INIT()     P4DIR |=  0x02 // P4.1
#define DEBUG_PIN_SLOT_TOGGLE()   P4OUT ^=  0x02
#define DEBUG_PIN_SLOT_CLR()      P4OUT &= ~0x02
#define DEBUG_PIN_SLOT_SET()      P4OUT |=  0x02

#define DEBUG_PIN_FSM_INIT()      P4DIR |=  0x04 // P4.2
#define DEBUG_PIN_FSM_TOGGLE()    P4OUT ^=  0x04
#define DEBUG_PIN_FSM_CLR()       P4OUT &= ~0x04
#define DEBUG_PIN_FSM_SET()       P4OUT |=  0x04

#define DEBUG_PIN_TASK_INIT()     P4DIR |=  0x08 // P4.3
#define DEBUG_PIN_TASK_TOGGLE()   P4OUT ^=  0x08
#define DEBUG_PIN_TASK_CLR()      P4OUT &= ~0x08
#define DEBUG_PIN_TASK_SET()      P4OUT |=  0x08

#define DEBUG_PIN_ISR_INIT()      P4DIR |=  0x10 // P4.4
#define DEBUG_PIN_ISR_TOGGLE()    P4OUT ^=  0x10
#define DEBUG_PIN_ISR_CLR()       P4OUT &= ~0x10
#define DEBUG_PIN_ISR_SET()       P4OUT |=  0x10

#define DEBUG_PIN_RADIO_INIT()    P1DIR |=  0x02 // P1.1
#define DEBUG_PIN_RADIO_TOGGLE()  P1OUT ^=  0x02
#define DEBUG_PIN_RADIO_CLR()     P1OUT &= ~0x02
#define DEBUG_PIN_RADIO_SET()     P1OUT |=  0x02

__no_init volatile uint8_t eui64 @ 0x10ee;       // address is flash where the node's EUI64 identifier is stored

enum {
   TRUE  = 1,
   FALSE = 0,
};

enum {
   HOPPING_ENABLED                 =  FALSE,
   SCHEDULELENGTH                  =      5,
   MAXNUMNEIGHBORS                 =     10,
   DEFAULTCHANNEL                  =     15,
   TXRETRIES                       =      3,
   //state
   QUEUELENGTH                     =     10,
   SERIAL_OUTPUT_BUFFER_SIZE       =    300,
   SERIAL_INPUT_BUFFER_SIZE        =    200,     //not more than 255 (length encoded in 1B)
   //misc
   GOODNEIGHBORMINPOWER            =    219,     //-80dBm in 8-bit 2's compl. (-80 -> -35 -> 36 -> 219)
   BADNEIGHBORMAXPOWER             =    229,     //-70dBm in 8-bit 2's compl. (-70 -> -25 -> 26 -> 0001 1010 -> 1110 0101-> 229)
   SWITCHSTABILITYTHRESHOLD        =      3,
   TX_POWER                        =     31,     //1=-25dBm, 31=0dBm (max value)
   MAXPREFERENCE                   =      2,
};

typedef uint16_t  slotOffset_t;
typedef uint16_t  shortnodeid_t;
typedef uint64_t  longnodeid_t;
typedef uint32_t  timervalue_t;
typedef uint16_t  errorparameter_t;
typedef uint8_t   dagrank_t;
typedef uint16_t  asn_t;

typedef uint8_t   error_t;

enum {
   E_SUCCESS      =  0,          
   E_FAIL         =  1,
};

enum {
   ADDR_NONE   = 0,
   ADDR_16B    = 1,
   ADDR_64B    = 2,
   ADDR_128B   = 3,
   ADDR_PANID  = 4,
   ADDR_PREFIX = 5,
};

typedef struct open_addr_t {                     //always written big endian, i.e. MSB in addr[0]
   uint8_t type;
   union {
      uint8_t addr_16b[2];
      uint8_t addr_64b[8];
      uint8_t addr_128b[16];
      uint8_t panid[2];
      uint8_t prefix[8];
   };
} open_addr_t;

enum {
   LITTLE_ENDIAN = TRUE,
   BIG_ENDIAN    = FALSE,
};

enum {
   IS_ADV        = TRUE,
   IS_NOT_ADV    = FALSE,
};

enum {
   IANA_UNDEFINED                     = 0x00,
   IANA_TCP                           = 0x06,
   IANA_UDP                           = 0x11,
   IANA_ICMPv6                        = 0x3a,
   IANA_ICMPv6_ECHO_REQUEST           =  128,
   IANA_ICMPv6_ECHO_REPLY             =  129,
   IANA_ICMPv6_RS                     =  133,
   IANA_ICMPv6_RA                     =  134,
   IANA_ICMPv6_RA_PREFIX_INFORMATION  =    3,
   IANA_ICMPv6_RPL                    =  155,
   IANA_ICMPv6_RPL_DIO                = 0x01,
};

enum {
   //TCP
   WKP_TCP_ECHO       =    7,
   WKP_TCP_INJECT     = 2188,
   WKP_TCP_HTTP       =   80,
   WKP_TCP_DISCARD    =    9,
   //UDP
   WKP_UDP_CHANNEL    = 2191,
   WKP_UDP_ECHO       =    7,
   WKP_UDP_GINA       = 2190,
   WKP_UDP_HELI       = 2192,
   WKP_UDP_LEDS       = 2193,
   WKP_UDP_INJECT     = 2188,
   WKP_UDP_TIMER      = 2195,
   WKP_UDP_DISCARD    =    9,
   WKP_UDP_SENSOR     = 2189,
   WKP_UDP_WARPWING   = 2194,
};

//OpenQueue entry definition
typedef struct OpenQueueEntry_t {
   //admin
   uint8_t       creator;                        //the component which called getFreePacketBuffer()
   uint8_t       owner;                          //the component which currently owns the entry
   uint8_t*      payload;                        //pointer to the start of the payload within 'packet'
   uint8_t       length;                         //length in bytes of the payload
   //l4
   uint8_t       l4_protocol;                    //l4 protocol to be used
   uint16_t      l4_sourcePortORicmpv6Type;      //l4 source port
   uint16_t      l4_destination_port;            //l4 destination port
   uint8_t*      l4_payload;                     //pointer to the start of the payload of l4 (used for retransmits)
   uint8_t       l4_length;                      //length of the payload of l4 (used for retransmits)
   //l3
   open_addr_t   l3_destinationORsource;         //128b IPv6 destination (down stack) or source address (up)
   //l2
   open_addr_t   l2_nextORpreviousHop;           //64b IEEE802.15.4 next (down stack) or previous (up) hop address
   uint8_t       l2_frameType;                   //beacon, data, ack, cmd
   uint8_t       l2_retriesLeft;
   //l1 (drivers)
   uint8_t       l1_txPower;
   uint8_t       l1_rssi;
   uint8_t       l1_lqi;
   bool          l1_crc;
   uint32_t      l1_rxTimestamp;
   //the packet
   uint8_t       packet[1+1+125+2+1];            // 1B spi address, 1B length, 125B data, 2B CRC, 1B LQI
} OpenQueueEntry_t;

//component identifiers
enum {
   COMPONENT_NULL             = 0x00,
   //l7
   COMPONENT_APPTCPECHO       = 0x01,
   COMPONENT_APPTCPINJECT     = 0x02,
   COMPONENT_APPTCPOHLONE     = 0x03,
   COMPONENT_APPTCPPRINT      = 0x04,
   COMPONENT_APPUDPCHANNEL    = 0x05,
   COMPONENT_APPUDPECHO       = 0x06,
   COMPONENT_APPUDPGINA       = 0x07,
   COMPONENT_APPUDPHELI       = 0x08,
   COMPONENT_APPUDPINJECT     = 0x09,
   COMPONENT_APPUDPTIMER      = 0x0a,
   COMPONENT_APPUDPLEDS       = 0x0b,
   COMPONENT_APPUDPPRINT      = 0x0c,
   COMPONENT_APPUDPSENSOR     = 0x0d,
   //l4
   COMPONENT_TCP              = 0x0e,             
   COMPONENT_UDP              = 0x0f,
   //l3b
   COMPONENT_FORWARDING       = 0x10,
   COMPONENT_ICMPv6           = 0x11,
   COMPONENT_ICMPv6ECHO       = 0x12,
   COMPONENT_ICMPv6ROUTER     = 0x13,
   COMPONENT_ICMPv6RPL        = 0x14,
   //l3a
   COMPONENT_OPENBRIDGE       = 0x15,
   COMPONENT_IPHC             = 0x16,
   //l2b
   COMPONENT_RES              = 0x17,
   COMPONENT_NEIGHBORS        = 0x18,
   COMPONENT_SCHEDULE         = 0x19,
   //l2a
   COMPONENT_MAC              = 0x1a,
   //phy
   COMPONENT_RADIO            = 0x1b,
   //cross-layer
   COMPONENT_IDMANAGER        = 0x1c,
   COMPONENT_OPENQUEUE        = 0x1d,
   COMPONENT_OPENSERIAL       = 0x1e,
   COMPONENT_PACKETFUNCTIONS  = 0x1f,
};

//============================================ debug ======================================

//status elements
enum {
   STATUS_RES_DAGRANK                    = 0,
   STATUS_SCHEDULE_CELLTABLE             = 1,
   STATUS_NEIGHBORS_NEIGHBORS            = 2,
   STATUS_OPENSERIAL_OUTPUTBUFFERINDEXES = 3,
   STATUS_OPENQUEUE_QUEUE                = 4,
   STATUS_IDMANAGER_ID                   = 5,
};

//error codes
enum {
   // l7
   ERR_SENDDONE                   = 0x01, // sendDone                          [AppP]    arg1=error arg2=ack   
   ERR_RCVD_ECHO_REQUEST          = 0x02, // received echo request             [RPLC]
   ERR_RCVD_ECHO_REPLY            = 0x03, // received echo reply               [RPLC]
   ERR_GETDATA_ASKS_TOO_FEW_BYTES = 0x04, // getData asks too few bytes        [SerialIO] arg1=maxNumBytes arg2=input_buffer_fill_level
   ERR_INPUT_BUFFER_OVERFLOW      = 0x05, // input buffer overflow             [SerialIO]
   // l4
   ERR_WRONG_TRAN_PROTOCOL        = 0x06, // wrong transport protocol          [App] arg=tran_protocol
   ERR_WRONG_TCP_STATE            = 0x07, // wrong TCP state                   [TCP] arg=state arg2=location
   ERR_RESET                      = 0x08, // TCP reset                         [TCP] arg=state arg2=location
   ERR_UNSUPPORTED_PORT_NUMBER    = 0x09, // unsupported port number           [all Apps and transport protocols] arg1=portNumber
   // l3
   ERR_UNSUPPORTED_ICMPV6_TYPE    = 0x0a, // unsupported ICMPv6 type           [RPLC] arg1=icmpv6_type arg2=location
   ERR_6LOWPAN_UNSUPPORTED        = 0x0b, // unsupported 6LoWPAN parameter     [IPHC] arg1=location arg2=param
   ERR_NO_NEXTHOP                 = 0x0c, // no nextHop                        [RPL]   
   // l2
   ERR_WRONG_STATE_IN_STARTSLOT   = 0x0d, // wrong state in startSlot          [IEEE802154EP]  arg1=state arg2=slotOffset
   ERR_WRONG_STATE_IN_TIMERFIRES  = 0x0e, // wrong state in timer fires        [IEEE154E] arg1=state, arg2=slotOffset  
   ERR_WRONG_STATE_IN_NEWSLOT     = 0x0f, // wrong state in start of frame     [IEEE154E] arg1=state,  arg2=slotOffset
   ERR_WRONG_STATE_IN_ENDOFFRAME  = 0x10, // wrong state in end of frame       [IEEE154E] arg1=state, arg2=slotOffset
   ERR_MAXTXDATAPREPARE_OVERFLOW  = 0x11, // maxTxDataPrepare overflows        [IEEE154E] arg1=state, arg2=slotOffset
   ERR_MAXRXACKPREPARE_OVERFLOWS  = 0x12, // maxRxAckPrepapre overflows        [IEEE154E] arg1=state, arg2=slotOffset
   ERR_MAXRXDATAPREPARE_OVERFLOWS = 0x13, // maxRxDataPrepapre overflows       [IEEE154E] arg1=state, arg2=slotOffset
   ERR_MAXTXACKPREPARE_OVERFLOWS  = 0x14, // maxTxAckPrepapre overflows        [IEEE154E] arg1=state, arg2=slotOffset   
   ERR_WDDATADURATION_OVERFLOWS   = 0x15, // wdDataDuration overflows          [IEEE154E] arg1=state, arg2=slotOffset
   ERR_WDRADIO_OVERFLOW           = 0x16, // wdRadio overflows                 [IEEE154E] arg1=state, arg2=slotOffset
   ERR_WDRADIOTX_OVERFLOWS        = 0x17, // wdRadioTx overflows               [IEEE154E] arg1=state, arg2=slotOffset
   ERR_WDACKDURATION_OVERFLOWS    = 0x18, // wdAckDuration overflows           [IEEE154E] arg1=state, arg2=slotOffset   
   ERR_IEEE154_UNSUPPORTED        = 0x19, // unsupported 802154 parameter      [IEEE802154EP] arg1=location arg2=param   
   ERR_NEIGHBORS_FULL             = 0x1a, // neighbors table is full           [NeighborsP] arg1=MAXNUMNEIGHBORS
   ERR_WRONG_CELLTYPE             = 0x1b, // wrong celltype                    [Schedule,IEEE802154EP,OpenQueueP] arg1=type      
   // drivers
   ERR_WRONG_IRQ_STATUS           = 0x1c, // wrong IRQ_STATUS                  [radio] arg1=irq_status
   // general
   ERR_BUSY_SENDING               = 0x1d, // busy sending a packet             [RPLP,TCPP] arg1=location
   ERR_UNEXPECTED_SENDDONE        = 0x1e, // sendDone for packet I didn't send [App,Advertise,KeepAlive,Reservation]
   ERR_NO_FREE_PACKET_BUFFER      = 0x1f, // no free Queuepkt Cell             [NeighborsP, NRESP, AppSensorP, IEEE802154EP] arg1=codeLocation
   ERR_UNSUPPORTED_COMMAND        = 0x20, // unsupported command=arg1          [SerialIOP] arg1=command
   ERR_MSG_UNKNOWN_TYPE           = 0x21, // received message of unknown type  [NRESC,OpenQueueP] arg1=type
   ERR_WRONG_ADDR_TYPE            = 0x22, // wrong address type                [IEEE802154EP,IDManagerP,PacketFunctions] arg1=addressType arg2=codeLocation
   ERR_BRIDGE_MISMATCH            = 0x23, // isBridge mismatch                 [NRES] arg1=code_location
   ERR_HEADER_TOO_LONG            = 0x24, // header too long                   [PacketFunctions] arg1=code_location
   ERR_INPUTBUFFER_LENGTH         = 0x25, // input length problem              [openSerial, all components which get Triggered] arg1=input_buffer_length arg2=location   
};

//=========================== global variable =================================

extern uint8_t openwsn_frequency_channel;

//=========================== prototypes ======================================

void openwsn_init();

#endif
