/*
 * UDP Print application
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef __APPUDPPRINT_H
#define __APPUDPPRINT_H

void appudpprint_init();
void appudpprint_sendDone(OpenQueueEntry_t* msg, error_t error);
void appudpprint_receive(OpenQueueEntry_t* msg);
bool appudpprint_debugPrint();

#endif
