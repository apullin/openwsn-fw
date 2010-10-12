/*
 * TCP Echo application
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef __APPTCPECHO_H
#define __APPTCPECHO_H

void apptcpecho_init();
bool apptcpecho_shouldIlisten();
void apptcpecho_receive(OpenQueueEntry_t* msg);
void apptcpecho_sendDone(OpenQueueEntry_t* msg, error_t error);
void apptcpecho_connectDone();
bool apptcpecho_debugPrint();

#endif
