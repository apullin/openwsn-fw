/**
\brief Implementation of RES

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/

#ifndef __RES_H
#define __RES_H

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void    res_init();
error_t res_send(OpenQueueEntry_t *msg);
void    res_sendDone(OpenQueueEntry_t* msg, error_t error);
void    res_receive(OpenQueueEntry_t* msg);
bool    res_debugPrint();

#endif