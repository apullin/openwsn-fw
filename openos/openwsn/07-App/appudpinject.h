/**
\brief UDP Inject application

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/

#ifndef __APPUDPINJECT_H
#define __APPUDPINJECT_H

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void appudpinject_init();
void appudpinject_trigger();
void appudpinject_sendDone(OpenQueueEntry_t* msg, error_t error);
void appudpinject_receive(OpenQueueEntry_t* msg);
bool appudpinject_debugPrint();

#endif
