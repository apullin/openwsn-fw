#include "openwsn.h"
#include "appudptimer.h"
#include "udp.h"
#include "openqueue.h"
#include "openserial.h"
#include "packetfunctions.h"
#include "timers.h"
#include "idmanager.h"

//=========================== variables =======================================

typedef struct {
   bool busySending;
} appudptimer_vars_t;

appudptimer_vars_t appudptimer_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

void appudptimer_init() {
   appudptimer_vars.busySending = FALSE;
   timer_startPeriodic(TIMER_UDPTIMER,32768);
}

void timer_appudptimer_fired() {
   OpenQueueEntry_t* pkt;
   // only send a packet if I received a sendDone for the previous.
   // the packet might be stuck in the queue for a long time for
   // example while the mote is synchronizing
   if (appudptimer_vars.busySending==FALSE) {
      //prepare packet
      pkt = openqueue_getFreePacketBuffer();
      if (pkt==NULL) {
         openserial_printError(COMPONENT_APPUDPTIMER,ERR_NO_FREE_PACKET_BUFFER,(errorparameter_t)0,(errorparameter_t)0);
         return;
      }
      pkt->creator                               = COMPONENT_APPUDPTIMER;
      pkt->owner                                 = COMPONENT_APPUDPTIMER;
      pkt->l4_protocol                           = IANA_UDP;
      pkt->l4_sourcePortORicmpv6Type             = WKP_UDP_TIMER;
      pkt->l4_destination_port                   = WKP_UDP_TIMER;
      pkt->l3_destinationORsource.type           = ADDR_128B;
      pkt->l3_destinationORsource.addr_128b[ 0]  = 0xde;
      pkt->l3_destinationORsource.addr_128b[ 1]  = 0xad;
      pkt->l3_destinationORsource.addr_128b[ 2]  = 0xbe;
      pkt->l3_destinationORsource.addr_128b[ 3]  = 0xef;
      pkt->l3_destinationORsource.addr_128b[ 4]  = 0xfa;
      pkt->l3_destinationORsource.addr_128b[ 5]  = 0xce;
      pkt->l3_destinationORsource.addr_128b[ 6]  = 0xca;
      pkt->l3_destinationORsource.addr_128b[ 7]  = 0xfe;
      pkt->l3_destinationORsource.addr_128b[ 8]  = 0x14;
      pkt->l3_destinationORsource.addr_128b[ 9]  = 0x15;
      pkt->l3_destinationORsource.addr_128b[10]  = 0x92;
      pkt->l3_destinationORsource.addr_128b[11]  = 0x09;
      pkt->l3_destinationORsource.addr_128b[12]  = 0x02;
      pkt->l3_destinationORsource.addr_128b[13]  = 0x2c;
      pkt->l3_destinationORsource.addr_128b[14]  = 0x00;
      if (idmanager_getMyID(ADDR_16B)->addr_16b[1]==DEBUG_MOTEID_MASTER) {
         pkt->l3_destinationORsource.addr_128b[15]  = DEBUG_MOTEID_SLAVE;
      } else {
         pkt->l3_destinationORsource.addr_128b[15]  = DEBUG_MOTEID_MASTER;
      }
      packetfunctions_reserveHeaderSize(pkt,76);// this yield a full-length packet
      ((uint8_t*)pkt->payload)[0]                = 'p';
      ((uint8_t*)pkt->payload)[1]                = 'o';
      ((uint8_t*)pkt->payload)[2]                = 'i';
      ((uint8_t*)pkt->payload)[3]                = 'p';
      ((uint8_t*)pkt->payload)[4]                = 'o';
      ((uint8_t*)pkt->payload)[5]                = 'i';
      //send packet
      if ((udp_send(pkt))==E_FAIL) {
         openqueue_freePacketBuffer(pkt);
      }
      appudptimer_vars.busySending               = TRUE;
   }
}

void appudptimer_sendDone(OpenQueueEntry_t* msg, error_t error) {
   msg->owner = COMPONENT_APPUDPTIMER;
   if (msg->creator!=COMPONENT_APPUDPTIMER) {
      openserial_printError(COMPONENT_APPUDPTIMER,ERR_UNEXPECTED_SENDDONE,0,0);
   }
   openqueue_freePacketBuffer(msg);
   appudptimer_vars.busySending = FALSE;
}

void appudptimer_receive(OpenQueueEntry_t* msg) {
   openqueue_freePacketBuffer(msg);
}

bool appudptimer_debugPrint() {
   return FALSE;
}

//=========================== private =========================================