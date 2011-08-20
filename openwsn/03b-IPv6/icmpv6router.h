/**
\brief ICMPv6 router advertisement implementation

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/

#ifndef __ICMPv6ROUTER_H
#define __ICMPv6ROUTER_H

/**
\addtogroup IPv6
\{
\addtogroup ICMPv6Router
\{
*/

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void icmpv6router_init();
void icmpv6router_trigger();
void icmpv6router_sendDone(OpenQueueEntry_t* msg, error_t error);
void icmpv6router_receive(OpenQueueEntry_t* msg);
bool icmpv6router_debugPrint();

/**
\}
\}
*/

#endif
