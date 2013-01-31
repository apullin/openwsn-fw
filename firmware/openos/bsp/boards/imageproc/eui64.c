/**
\brief TelosB-specific definition of the "eui64" bsp module.

\author Andrew Pullin <pullin@berkeley.edu>, January 2013.
*/

#include "p33fj128mc706a.h"
#include "stdint.h"
#include "string.h"
#include "eui64.h"

//=========================== defines =========================================

//=========================== variables =======================================

//Current implementation just uses baked-in address.
//Maybe do this with a pre-build python script that generates a .h file?
//static unsigned char eui64[] = {0x12,0x34,0x56,0x78,0xab,0xcd,0xef,0x00};
uint64_t eui64int = 0x0012345678abcdef;
static char* eui64 = (char*)(&eui64int);

//=========================== prototypes ======================================

//=========================== public ==========================================

void eui64_get(uint8_t* addressToWrite) {
   memcpy(addressToWrite,(void const*)eui64,8);
}

//=========================== private =========================================