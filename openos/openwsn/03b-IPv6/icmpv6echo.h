/*
 * ICMPv6 echo (ping) implementation
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef __ICMPv6ECHO_H
#define __ICMPv6ECHO_H

void icmpv6echo_init();
void icmpv6echo_trigger();
void icmpv6echo_sendDone(OpenQueueEntry_t* msg, error_t error);
void icmpv6echo_receive(OpenQueueEntry_t* msg);
bool icmpv6echo_debugPrint();

#endif
