/**
\brief ICMPv6 RPL implementation

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/

#ifndef __ICMPv6RPL_H
#define __ICMPv6RPL_H

/**
\addtogroup IPv6
\{
\addtogroup ICMPv6RPL
\{
*/

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void icmpv6rpl_init();
void icmpv6rpl_trigger();
void icmpv6rpl_sendDone(OpenQueueEntry_t* msg, error_t error);
void icmpv6rpl_receive(OpenQueueEntry_t* msg);
bool icmpv6rpl_debugPrint();

/**
\}
\}
*/

#endif
