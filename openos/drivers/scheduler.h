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
   ID_ISR_RADIO               = 0, // interrupt through IRQ_RF connected to P1.6
   ID_ISR_RPL                 = 1, // timerB CCR0
   ID_ISR_TCP_TIMEOUT         = 2, // timerB CCR1
   ID_ISR_TIMERB2             = 3, // timerB CCR2
   ID_ISR_TIMERB3             = 4, // timerB CCR3
   ID_ISR_TIMERB4             = 5, // timerB CCR4
   ID_ISR_TIMERB5             = 6, // timerB CCR5
   ID_ISR_TIMERB6             = 7, // timerB CCR6
   ID_ISR_BUTTON              = 8, // P2.7
   MAX_NUM_TASKS              = 9,
};

//===================================== prototypes ============================

void scheduler_init();
void scheduler_start();
void scheduler_push_task(int8_t task_id);
#ifdef OPENWSN_STACK
void tsch_newSlot();
#endif
#ifdef OPENWSN_STACK
void tsch_timerFires();
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