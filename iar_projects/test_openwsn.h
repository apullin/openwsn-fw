/*
 * This is a sandbox project to develop OpenWSN onto the GINA platform.
 *
 * Author:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef __TEST_OPENWSN_H
#define __TEST_OPENWSN_H

#include "openwsn.h"
#include "iphc.h"

void bridge_sendDone(OpenQueueEntry_t* pkt, error_t error);
void bridge_receive(OpenQueueEntry_t* packetReceived);

#endif

