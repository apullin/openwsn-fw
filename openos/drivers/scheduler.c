/**
\brief Minimal FCFS scheduler for the GINA2.2b/c boards.

\author Ankur Mehta <watteyne@eecs.berkeley.edu>, October 2010
\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/

//ISR_RADIO
//ISR_BUTTON
//ISR_TIMERS
//ISR_SPI
//ISR_I2C
//ISR_SERIAL
//ISR_ADC
//OPENWSN_STACK

#include "scheduler.h"
#include "timers.h"
#include "ieee154etimer.h"
#include "IEEE802154E.h"
#include "i2c.h"
#include "openserial.h"

//=========================== variables =======================================

typedef struct {
   uint8_t task_list[MAX_NUM_TASKS];
   uint8_t index_first_task;
   uint8_t num_tasks;
} scheduler_vars_t;

scheduler_vars_t scheduler_vars;

extern timers_vars_t timers_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

void scheduler_init() {
   uint8_t task_counter;
   scheduler_vars.index_first_task = 0;
   scheduler_vars.num_tasks        = 0;
   DEBUG_PIN_TASK_INIT();
   DEBUG_PIN_ISR_INIT();
   for (task_counter=0;task_counter<MAX_NUM_TASKS;task_counter++) {
      scheduler_vars.task_list[task_counter] = 0;
   }
}

void scheduler_start() {
   while (1) {                                   // IAR should halt here if nothing to do
      while(scheduler_vars.num_tasks>0) {
         if (scheduler_vars.task_list[TASKID_RES]>0) {
            scheduler_vars.task_list[TASKID_RES]--;
            scheduler_vars.num_tasks--;
#ifdef OPENWSN_STACK
            timer_res_fired();
#endif
         } else if (scheduler_vars.task_list[TASKID_RPL]>0) {
            scheduler_vars.task_list[TASKID_RPL]--;
            scheduler_vars.num_tasks--;
#ifdef OPENWSN_STACK
            timer_rpl_fired();
#endif
         } else if (scheduler_vars.task_list[TASKID_TCP_TIMEOUT]>0) {
            scheduler_vars.task_list[TASKID_TCP_TIMEOUT]--;
            scheduler_vars.num_tasks--;
#ifdef OPENWSN_STACK
            timer_tcp_fired();
#endif
         } else if (scheduler_vars.task_list[TASKID_UDP_TIMER]>0) {
            scheduler_vars.task_list[TASKID_UDP_TIMER]--;
            scheduler_vars.num_tasks--;
#ifdef OPENWSN_STACK
            timer_appudptimer_fired();
#endif
         } else if (scheduler_vars.task_list[TASKID_TIMERB4]>0) {
            scheduler_vars.task_list[TASKID_TIMERB4]--;
            scheduler_vars.num_tasks--;
            // timer available, put your function here
         } else if (scheduler_vars.task_list[TASKID_TIMERB5]>0) {
            scheduler_vars.task_list[TASKID_TIMERB5]--;
            scheduler_vars.num_tasks--;
            // timer available, put your function here
         } else if (scheduler_vars.task_list[TASKID_TIMERB6]>0) {
            scheduler_vars.task_list[TASKID_TIMERB6]--;
            scheduler_vars.num_tasks--;
            // timer available, put your function here
         } else if (scheduler_vars.task_list[TASKID_BUTTON]>0) {
            scheduler_vars.task_list[TASKID_BUTTON]--;
            scheduler_vars.num_tasks--;
#ifdef ISR_BUTTON
            isr_button();
#endif
         }
      }
      DEBUG_PIN_TASK_CLR();
      __bis_SR_register(GIE+LPM3_bits);          // sleep, but leave interrupts and ACLK on 
      DEBUG_PIN_TASK_SET();
   }
}

void scheduler_push_task(int8_t task_id) {
   if(task_id>=MAX_NUM_TASKS) { while(1); }
   scheduler_vars.task_list[task_id]++;
   scheduler_vars.num_tasks++;
}

//=========================== interrupt handlers ==============================

//======= handled as tasks

#pragma vector = PORT2_VECTOR
__interrupt void PORT2_ISR (void) {
   CAPTURE_TIME();
   DEBUG_PIN_ISR_SET();
#ifdef ISR_BUTTON
   //interrupt from button connected to P2.7
   if ((P2IFG & 0x80)!=0) {
      P2IFG &= ~0x80;                            // clear interrupt flag
      scheduler_push_task(ID_ISR_BUTTON);        // post task
      __bic_SR_register_on_exit(CPUOFF);         // restart CPU
   }
#endif
   DEBUG_PIN_ISR_CLR();
}

// TimerB CCR0 interrupt service routine
#pragma vector = TIMERB0_VECTOR
__interrupt void TIMERB0_ISR (void) {
   CAPTURE_TIME();
   DEBUG_PIN_ISR_SET();
#ifdef ISR_TIMERS
   if (timers_vars.continuous[0]==TRUE) {
      TBCCR0 += timers_vars.period[0];           // continuous timer: schedule next instant
   } else {
      TBCCTL0 = 0;                               // stop the timer
      TBCCR0  = 0;
   }
   scheduler_push_task(TASKID_RES);              // post the corresponding task
   __bic_SR_register_on_exit(CPUOFF);            // restart CPU
#endif
   DEBUG_PIN_ISR_CLR();
}

// TimerB CCR1-6 interrupt service routine
#pragma vector = TIMERB1_VECTOR
__interrupt void TIMERB1through6_ISR (void) {
   CAPTURE_TIME();
   DEBUG_PIN_ISR_SET();
#ifdef ISR_TIMERS
   uint16_t tbiv_temp = TBIV;                    // read only once because accessing TBIV resets it
   switch (tbiv_temp) {
      case 0x0002: // timerB CCR1
         if (timers_vars.continuous[1]==TRUE) {
            TBCCR1 += timers_vars.period[1];     // continuous timer: schedule next instant
         } else {
            TBCCTL1 = 0;                         // stop the timer
            TBCCR1  = 0;
         }
         scheduler_push_task(TASKID_RPL);        // post the corresponding task
         __bic_SR_register_on_exit(CPUOFF);      // restart CPU
         break;
      case 0x0004: // timerB CCR2
         if (timers_vars.continuous[2]==TRUE) {
            TBCCR2 += timers_vars.period[2];     // continuous timer: schedule next instant
         } else {
            TBCCTL2 = 0;                         // stop the timer
            TBCCR2  = 0;
         }
         scheduler_push_task(TASKID_TCP_TIMEOUT);// post the corresponding task
         __bic_SR_register_on_exit(CPUOFF);      // restart CPU
         break;
      case 0x0006: // timerB CCR3
         if (timers_vars.continuous[3]==TRUE) {
            TBCCR3 += timers_vars.period[3];     // continuous timer: schedule next instant
         } else {
            TBCCTL3 = 0;                         // stop the timer
            TBCCR3  = 0;
         }
         scheduler_push_task(TASKID_UDP_TIMER);  // post the corresponding task
         __bic_SR_register_on_exit(CPUOFF);      // restart CPU
         break;
      case 0x0008: // timerB CCR4
         if (timers_vars.continuous[4]==TRUE) {
            TBCCR4 += timers_vars.period[4];     // continuous timer: schedule next instant
         } else {
            TBCCTL4 = 0;                         // stop the timer
            TBCCR4  = 0;
         }
         scheduler_push_task(TASKID_TIMERB4);    // post the corresponding task
         __bic_SR_register_on_exit(CPUOFF);      // restart CPU
         break;
      case 0x000A: // timerB CCR5
         if (timers_vars.continuous[5]==TRUE) {
            TBCCR5 += timers_vars.period[5];     // continuous timer: schedule next instant
         } else {
            TBCCTL5 = 0;                         // stop the timer
            TBCCR5  = 0;
         }
         scheduler_push_task(TASKID_TIMERB5);    // post the corresponding task
         __bic_SR_register_on_exit(CPUOFF);      // restart CPU
         break;
      case 0x000C: // timerB CCR6
         if (timers_vars.continuous[6]==TRUE) {
            TBCCR6 += timers_vars.period[6];     // continuous timer: schedule next instant
         } else {
            TBCCTL6 = 0;                         // stop the timer
            TBCCR6  = 0;
         }
         scheduler_push_task(TASKID_TIMERB6);    // post the corresponding task
         __bic_SR_register_on_exit(CPUOFF);      // restart CPU
         break;
      default:
         while(1);                               // this should not happen
   }
#endif
   DEBUG_PIN_ISR_CLR();
}

//======= handled in ISR mode

#pragma vector = PORT1_VECTOR
__interrupt void PORT1_ISR (void) {
    CAPTURE_TIME();
    DEBUG_PIN_ISR_SET();
#ifdef ISR_RADIO
   //interrupt from radio through IRQ_RF connected to P1.6
   if ((P1IFG & 0x40)!=0) {
      P1IFG &= ~0x40;                            // clear interrupt flag
      isr_radio();
   }
#endif
   DEBUG_PIN_ISR_CLR();
}

// TimerA CCR0 interrupt service routine
#pragma vector = TIMERA0_VECTOR
__interrupt void TIMERA0_ISR (void) {
   CAPTURE_TIME();
   DEBUG_PIN_ISR_SET();
#ifdef OPENWSN_STACK
   isr_ieee154e_newSlot();
#endif
   DEBUG_PIN_ISR_CLR();
}

// TimerA CCR1-2 interrupt service routine
#pragma vector = TIMERA1_VECTOR
__interrupt void TIMERA1and2_ISR (void) {
   CAPTURE_TIME();
   DEBUG_PIN_ISR_SET();
#ifdef OPENWSN_STACK
   uint16_t taiv_temp = TAIV;                    // read only once because accessing TAIV resets it
   switch (taiv_temp) {
      case 0x0002: // timerA CCR1 compare
         isr_ieee154e_timer();
         break;
      default:
         while(1);                               // this should not happen
   }
#endif
   DEBUG_PIN_ISR_CLR();
}

/* 
 * The GINA board has three buses: I2C, SPI, UART. We handle the
 * related interrupts directly.
 *
 * UCA1 = serial
 * UCB1 = I2C
 * UCA0 = SPI
 */

#pragma vector = USCIAB1TX_VECTOR
__interrupt void USCIAB1TX_ISR(void) {
   CAPTURE_TIME();
   DEBUG_PIN_ISR_SET();
#ifdef ISR_I2C
   if ( ((UC1IFG & UCB1TXIFG) && (UC1IE & UCB1TXIE)) ||
        ((UC1IFG & UCB1RXIFG) && (UC1IE & UCB1RXIE)) ) {
      isr_i2c_tx(1);                         // implemented in I2C driver
   }
#endif
#ifdef ISR_SERIAL
   if ( (UC1IFG & UCA1TXIFG) && (UC1IE & UCA1TXIE) ){
      isr_openserial_tx();                       // implemented in serial driver
   }
#endif
   DEBUG_PIN_ISR_CLR();
}

#pragma vector = USCIAB1RX_VECTOR
__interrupt void USCIAB1RX_ISR(void) {
   CAPTURE_TIME();
   DEBUG_PIN_ISR_SET();
#ifdef ISR_I2C
   if ( ((UC1IFG & UCB1RXIFG) && (UC1IE & UCB1RXIE)) ||
         (UCB1STAT & UCNACKIFG) ) {
      isr_i2c_rx(1);                             // implemented in I2C driver
   }
#endif

#ifdef ISR_SERIAL
   if ( (UC1IFG & UCA1RXIFG) && (UC1IE & UCA1RXIE) ){
      isr_openserial_rx();                  // implemented in serial driver
   }
#endif
   DEBUG_PIN_ISR_CLR();
}

#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCIAB0RX_ISR (void) {
    CAPTURE_TIME();
    DEBUG_PIN_ISR_SET();
#ifdef ISR_SPI
   if ( (IFG2 & UCA0RXIFG) && (IE2 & UCA0RXIE) ) {
      isr_spi_rx();                         // implemented in SPI driver
   }
#endif
#ifdef ISR_I2C
   if ( ((IFG2 & UCB0RXIFG) && (IE2 & UCB0RXIE)) ||
        (UCB0STAT & UCNACKIFG) ) {
      isr_i2c_rx(0);                             // implemented in I2C driver
   }
#endif
   DEBUG_PIN_ISR_CLR();
}

//======= handled as CPUOFF

// TODO: this is bad practice, should redo, even a busy wait is better

#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR (void) {
    CAPTURE_TIME();
    DEBUG_PIN_ISR_SET();
#ifdef ISR_ADC
   ADC12IFG &= ~0x1F;                            // clear interrupt flags
   __bic_SR_register_on_exit(CPUOFF);            // restart CPU
#endif
   DEBUG_PIN_ISR_CLR();
}
