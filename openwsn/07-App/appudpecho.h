/*
 * UDP Echo application
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef __APPUDPECHO_H
#define __APPUDPECHO_H

void appudpecho_init();
void appudpecho_receive(OpenQueueEntry_t* msg);
void appudpecho_sendDone(OpenQueueEntry_t* msg, error_t error);
bool appudpecho_debugPrint();

#endif
