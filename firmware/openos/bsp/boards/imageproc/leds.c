/**
\brief ImageProc2.4-specific definition of the "leds" bsp module.

\author Andrew Pullin <pullin@berkeley.edu>, January 2013.
*/

#include "p33fj128mc706a.h"
#include "stdint.h"
#include "leds.h"
#include "utils.h" //frim imageproc-lib

//=========================== defines =========================================
#define LED_RED LED_1
#define LED_GREEN LED_2
#define LED_YELLOW LED_3
//=========================== variables =======================================

//=========================== prototypes ======================================

//=========================== public ==========================================

void    leds_init() {
    LED_RED = 0;
    LED_GREEN = 0;
    LED_YELLOW = 0;
}

// red = LED1 = P5.4
void    leds_error_on() {
    LED_RED = 1;
}
void    leds_error_off() {
   LED_RED = 0;
}
void    leds_error_toggle() {
    LED_RED ^= 1;
}
uint8_t leds_error_isOn() {
   return (uint8_t)(LED_RED);
}

void leds_error_blink() {
   uint8_t i;
   volatile uint16_t delay;
   leds_all_on();
   
   // blink error LED for ~10s
   for (i=0;i<80;i++) {
      leds_all_off();
      for (delay=0xffff;delay>0;delay--);
   }
}

// green = LED2 = P5.5
void    leds_radio_on() {
    LED_GREEN = 1;
}
void    leds_radio_off() {
   LED_GREEN = 0;
}
void    leds_radio_toggle() {
   LED_GREEN ^= 1;
}
uint8_t leds_radio_isOn() {
   return (uint8_t)(LED_GREEN);
}

// blue = LED3 = P5.6
void    leds_sync_on() {
   LED_YELLOW = 1;
}
void    leds_sync_off() {
   LED_YELLOW = 0;
}
void    leds_sync_toggle() {
   LED_YELLOW ^= 1;
}
uint8_t leds_sync_isOn() {
   return (uint8_t)(LED_YELLOW);
}

void    leds_debug_on() {
   // TelosB doesn't have a debug LED :(
}
void    leds_debug_off() {
   // TelosB doesn't have a debug LED :(
}
void    leds_debug_toggle() {
   // TelosB doesn't have a debug LED :(
}
uint8_t leds_debug_isOn() {
   // TelosB doesn't have a debug LED :(
   return 0;
}

void    leds_all_on() {
    LED_RED = 1;
    LED_GREEN = 1;
    LED_YELLOW = 1;
}
void    leds_all_off() {
    LED_RED = 0;
    LED_GREEN = 0;
    LED_YELLOW = 0;
}
void    leds_all_toggle() {
    LED_RED ^= 1;
    LED_GREEN ^= 1;
    LED_YELLOW ^= 1;
}

void    leds_circular_shift() {
    //TODO: implement leds_circular_shift() for ImageProc2.4
}

void    leds_increment() {
    //TODO: leds_increment() implement for ImageProc2.4
}

//=========================== private =========================================