/*
 * TCP Echo Inject
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef __APPTCPINJECT_H
#define __APPTCPINJECT_H

void apptcpinject_init();
bool apptcpinject_shouldIlisten();
void apptcpinject_trigger();
void apptcpinject_connectDone(error_t error);
void apptcpinject_sendDone(OpenQueueEntry_t* msg, error_t error);
void apptcpinject_receive(OpenQueueEntry_t* msg);
bool apptcpinject_debugPrint();

#endif
