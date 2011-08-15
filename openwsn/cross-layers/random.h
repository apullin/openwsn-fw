/**
\brief A pseudo-random number generator

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2011
*/

#ifndef __RANDOM_H
#define __RANDOM_H

#include "openwsn.h"

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void     random_init();
uint16_t random_get16b();

#endif