/**
\brief This program shows the use of both the "bsp_timer" and "radiotimer"
       bsp modules.

Since the bsp modules for different platforms have the same declaration, you
can use this project with any platform.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, May 2012.
*/


#include "board.h"
#include "debugpins.h"
#include "leds.h"
#include "bsp_timer.h"
#include "radiotimer.h"

#include "openserial.h"
#include "opentimers.h"
//=========================== defines =========================================

#define BSP_TIMER_PERIOD                   327
#define RADIOTIMER_OVERFLOW_PERIOD         327
#define RADIOTIMER_COMPARE_PERIOD          4
#define RADIOTIMER_NUM_COMPARES            4  
#define ISR_DELAY                           500

#define BUFFER_SIZE                          50
//=========================== variables =======================================

typedef struct {
   uint8_t  radiotimer_num_compares_left;
   uint16_t radiotimer_last_compare_val;
   uint8_t  data[BUFFER_SIZE];
} app_vars_t;

app_vars_t app_vars;

typedef struct {
   uint16_t bsp_timer_num_compare;
   uint16_t radiotimer_num_overflow;
   uint16_t radiotimer_num_compare;
} app_dbg_t;

app_dbg_t app_dbg;

//=========================== prototypes ======================================

void bsp_timer_cb_compare();
void radiotimer_cb_overflow();
void radiotimer_cb_compare();

//=========================== main ============================================

/**
\brief The program starts executing here.
*/
int mote_main(void)
{  
   uint8_t i;
   // initialize board
   board_init();
   opentimers_init();
   openserial_init();
   i=0;
   while (i<BUFFER_SIZE){
     app_vars.data[i]=i;
     i++;
   }
     
   // switch radio LED on
   leds_radio_on();
   
   // prepare bsp_timer
   bsp_timer_set_callback(bsp_timer_cb_compare);
   
   // prepare radiotimer
   radiotimer_setOverflowCb(radiotimer_cb_overflow);
   radiotimer_setCompareCb(radiotimer_cb_compare);
   
   // kick off first bsp_timer compare
   bsp_timer_scheduleIn(BSP_TIMER_PERIOD);
   
   // start periodic radiotimer overflow
   radiotimer_start(RADIOTIMER_OVERFLOW_PERIOD);
   
   // kick off first radiotimer compare
   app_vars.radiotimer_num_compares_left  = RADIOTIMER_NUM_COMPARES-1;
   app_vars.radiotimer_last_compare_val   = RADIOTIMER_COMPARE_PERIOD;
   radiotimer_schedule(app_vars.radiotimer_last_compare_val);
   
   while (1) {
      board_sleep();
   }
}

//=========================== callbacks =======================================

void bsp_timer_cb_compare() {
   // toggle pin
   debugpins_frame_toggle();
   
   // toggle error led
   leds_sync_toggle();
   
   // increment counter
   app_dbg.bsp_timer_num_compare++;
   
   // schedule again
   bsp_timer_scheduleIn(BSP_TIMER_PERIOD);
}

void radiotimer_cb_overflow() {
   volatile uint16_t delay;
   
   // toggle pin
   debugpins_slot_toggle();
   
   // switch radio LED on
   leds_error_toggle();
   leds_radio_off();
   
   // reset the counter for number of remaining compares
   app_vars.radiotimer_num_compares_left  = RADIOTIMER_NUM_COMPARES;
   app_vars.radiotimer_last_compare_val   = RADIOTIMER_COMPARE_PERIOD;
   radiotimer_schedule(app_vars.radiotimer_last_compare_val);
   
   // increment debug counter
   app_dbg.radiotimer_num_overflow++;
   
   // wait a bit
   //debugpins_isr_set();
   //for (delay=0;delay<ISR_DELAY;delay++);
   //debugpins_isr_clr();
   //start output
   openserial_startOutput();
}

void radiotimer_cb_compare() {
   // toggle pin
   debugpins_fsm_toggle();
   
   // toggle radio LED
   leds_radio_toggle();
   
   // schedule a next compare, if applicable
   app_vars.radiotimer_last_compare_val += RADIOTIMER_COMPARE_PERIOD;
   app_vars.radiotimer_num_compares_left--;
   if (app_vars.radiotimer_num_compares_left>0) {
      radiotimer_schedule(app_vars.radiotimer_last_compare_val);
      if (app_vars.radiotimer_num_compares_left==2) openserial_printData(app_vars.data, sizeof(app_vars.data));
   } else {
      radiotimer_cancel();
   }
   
   // increment debug counter
   app_dbg.radiotimer_num_compare++;
}