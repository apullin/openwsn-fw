/*
 * ICMPv6 common definitions
 *
 * Authors:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef __ICMPv6_H
#define __ICMPv6_H

typedef struct ICMPv6_ht {
   uint8_t     type;
   uint8_t     code;
   uint16_t    checksum;
   uint16_t    identifier;
   uint16_t    sequence_number;
} ICMPv6_ht;

typedef struct ICMPv6_RA_ht {
   uint8_t     type;
   uint8_t     code;
   uint16_t    checksum;
   uint8_t     hop_limit;
   uint8_t     flags;
   uint16_t    router_lifetime;
   uint32_t    reachable_time;
   uint32_t    retransmission_timer;
} ICMPv6_RA_ht;

typedef struct ICMPv6_64bprefix_option_ht {
   uint8_t     option_type;
   uint8_t     option_length;
   uint8_t     prefix_length;
   uint8_t     flags;
   uint32_t    valid_lifetime;
   uint32_t    preferred_lifetime;
   uint32_t    unused;
   uint8_t     prefix[16];
} ICMPv6_64bprefix_option_ht;

//===================================== prototypes ============================

void    icmpv6_init();
error_t icmpv6_send(OpenQueueEntry_t* msg);
void    icmpv6_sendDone(OpenQueueEntry_t* msg, error_t error);
void    icmpv6_receive(OpenQueueEntry_t* msg);
bool    icmpv6_debugPrint();

#endif
