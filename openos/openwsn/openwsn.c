/*
 * General OpenWSN definitions
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 * Ankur Mehta <mehtank@eecs.berkeley.edu>, September 2010
 */

#include "openwsn.h"
#include "scheduler.h"
#ifdef OPENWSN_STACK
//l7 TCP applications
#include "apptcpecho.h"
#include "apptcpinject.h"
#include "apptcpohlone.h"
#include "apptcpprint.h"
//l7 UDP applications
#include "appudpchannel.h"
#include "appudpecho.h"
#include "appudpgina.h"
#include "appudpheli.h"
#include "appudpleds.h"
#include "appudpinject.h"
#include "appudpprint.h"
#include "appudpsensor.h"
#include "appudpwarpwing.h"
//l4
#include "tcp.h"
#include "udp.h"
//l3b
#include "forwarding.h"
#include "icmpv6.h"
#include "icmpv6echo.h"
#include "icmpv6router.h"
#include "icmpv6rpl.h"
//l3a
#include "openbridge.h"
#include "iphc.h"
//l2b
#include "nores.h"
#include "neighbors.h"
//l2a
#include "IEEE802154E.h"
//l1
#include "radio.h"
//cross-layer
#include "idmanager.h"
#include "openqueue.h"
#include "openserial.h"
//misc
#include "timers.h"
#endif

//===================================== variables =============================

uint8_t openwsn_frequency_channel;

//===================================== prototypes ============================

void openwsn_init();

//===================================== public ================================

void openwsn_init() {
   openwsn_frequency_channel = DEFAULTCHANNEL;
   scheduler_init();
#ifdef OPENWSN_STACK
   timer_init();
   idmanager_init();
   openqueue_init();
   radio_init();
   openserial_init();
   mac_init();
   neighbors_init();
   nores_init();
   forwarding_init();
   openbridge_init();
   iphc_init();
   icmpv6_init();
   icmpv6echo_init();
   icmpv6router_init();
   icmpv6rpl_init();
   tcp_init();
   udp_init();
   apptcpecho_init();
   apptcpinject_init();
   apptcpohlone_init();
   apptcpprint_init();
   appudpchannel_init();
   appudpecho_init();
   appudpgina_init();
   appudpheli_init();
   appudpinject_init();
   appudpleds_init();
   appudpprint_init();
   appudpsensor_init();
   appudpwarpwing_init();
#endif
}

