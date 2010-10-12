/*
 * UDP Sensor application
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef __APPUDPSENSOR_H
#define __APPUDPSENSOR_H

void appudpsensor_init();
void appudpsensor_trigger();
void appudpsensor_sendDone(OpenQueueEntry_t* msg, error_t error);
void appudpsensor_receive(OpenQueueEntry_t* msg);
bool appudpsensor_debugPrint();

#endif
