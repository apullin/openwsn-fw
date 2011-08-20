/**
\brief OpenBridge allows any OpenWSN node to act like a bridge between
       the wireless sensor network and the Internet.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/

#ifndef __OPENBRIDGE_H
#define __OPENBRIDGE_H

/**
\addtogroup LoWPAN
\{
\addtogroup OpenBridge
\{
*/

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void openbridge_init();
void openbridge_trigger();
void openbridge_sendDone(OpenQueueEntry_t* msg, error_t error);
void openbridge_receive(OpenQueueEntry_t* msg);
bool openbridge_debugPrint();

/**
\}
\}
*/

#endif
