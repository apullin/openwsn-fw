/*
 * Minimal FCFS scheduler for the GINA2.2b/c boards.
 *
 * Authors:
 * Ankur Mehta <watteyne@eecs.berkeley.edu>, October 2010
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include "msp430x26x.h"
#include "stdint.h"
#include "radio.h"

//===================================== define ================================

enum {
   TASKID_RES           = 0, // schedule by timerB CCR0 interrupt
   TASKID_RPL           = 1, // schedule by timerB CCR1 interrupt
   TASKID_TCP_TIMEOUT   = 2, // schedule by timerB CCR2 interrupt
   TASKID_UDP_TIMER     = 3, // schedule by timerB CCR3 interrupt
   TASKID_TIMERB4       = 4, // schedule by timerB CCR4 interrupt
   TASKID_TIMERB5       = 5, // schedule by timerB CCR5 interrupt
   TASKID_TIMERB6       = 6, // schedule by timerB CCR6 interrupt
   TASKID_BUTTON        = 7, // schedule by P2.7 interrupt
   MAX_NUM_TASKS        = 8,
};

//===================================== prototypes ============================

void scheduler_init();
void scheduler_start();
void scheduler_push_task(int8_t task_id);
#ifdef OPENWSN_STACK
void ieee154e_newSlot();
#endif
#ifdef OPENWSN_STACK
void ieee154e_timerFires();
#endif
#ifdef ISR_ADC
void isr_adc();
#endif
#ifdef ISR_GYRO
void isr_gyro();
#endif
#ifdef ISR_RADIO
void isr_radio();
#endif
#ifdef ISR_LARGE_RANGE_ACCEL
void isr_large_range_accel();
#endif
#ifdef ISR_BUTTON
void isr_button();
#endif
#ifdef ISR_SPIRX
void isr_spirx();
#endif
#ifdef ISR_SPITX
void isr_spitx();
#endif
#ifdef ISR_I2CRX
void isr_i2crx();
#endif
#ifdef ISR_I2CTX
void isr_i2ctx();
#endif

#endif