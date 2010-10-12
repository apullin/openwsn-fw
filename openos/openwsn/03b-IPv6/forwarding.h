/*
 * Forwarding engine
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef __FORWARDING_H
#define __FORWARDING_H

#include "iphc.h"

void    forwarding_init();
error_t forwarding_send(OpenQueueEntry_t *msg);
void    forwarding_sendDone(OpenQueueEntry_t* msg, error_t error);
void    forwarding_receive(OpenQueueEntry_t* msg, ipv6_header_iht ipv6_header);
bool    forwarding_debugPrint();

#endif
