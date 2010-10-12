/*
 * Implementation of noRES
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef __NORES_H
#define __NORES_H

void    nores_init();
error_t nores_send(OpenQueueEntry_t *msg);
void    nores_sendDone(OpenQueueEntry_t* msg, error_t error);
void    nores_receive(OpenQueueEntry_t* msg);
bool    nores_debugPrint();

#endif