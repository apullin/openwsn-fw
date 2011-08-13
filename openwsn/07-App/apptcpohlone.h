/**
\brief Ohlone, world's smaller web server

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/

#ifndef __APPTCPOHLONE_H
#define __APPTCPOHLONE_H

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void apptcpohlone_init();
bool apptcpohlone_shouldIlisten();
void apptcpohlone_receive(OpenQueueEntry_t* msg);
void apptcpohlone_sendDone(OpenQueueEntry_t* msg, error_t error);
void apptcpohlone_connectDone(error_t error);
bool apptcpohlone_debugPrint();

#endif
