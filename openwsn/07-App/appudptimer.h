/**
\brief UDP Timer application

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/

#ifndef __APPUDPTIMER_H
#define __APPUDPTIMER_H

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void appudptimer_init();
void appudptimer_sendDone(OpenQueueEntry_t* msg, error_t error);
void appudptimer_receive(OpenQueueEntry_t* msg);
bool appudptimer_debugPrint();

#endif
