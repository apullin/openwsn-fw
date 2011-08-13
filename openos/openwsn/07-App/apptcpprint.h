/**
\brief TCP Echo Inject

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/

#ifndef __APPTCPPRINT_H
#define __APPTCPPRINT_H

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void apptcpprint_init();
bool apptcpprint_shouldIlisten();
void apptcpprint_receive(OpenQueueEntry_t* msg);
void apptcpprint_connectDone(error_t error);
void apptcpprint_sendDone(OpenQueueEntry_t* msg, error_t error);
bool apptcpprint_debugPrint();

#endif
