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

#define MAX_NUM_TASKS  10

enum {
   ID_ISR_RADIO               = 0,
   ID_ISR_MAC_BACKOFF         = 1,
   ID_ISR_MAC_WATCHDOG        = 2,
   ID_ISR_MAC_PERIODIC        = 3,
   ID_ISR_TCP_TIMEOUT         = 4,
   ID_ISR_RPL                 = 5,
   ID_ISR_TIMERB5             = 6,
   ID_ISR_TIMERB6             = 7,
   ID_ISR_BUTTON              = 8,
   ID_TASK_APPLICATION        = 9,
};

//===================================== prototypes ============================

void scheduler_init();
void scheduler_start();
void scheduler_push_task(int8_t task_id);
/*#ifdef ISR_ADC
void isr_adc();
#endif
#ifdef ISR_GYRO
void isr_gyro();
#endif*/
#ifdef ISR_RADIO
void isr_radio();
#endif
/*#ifdef ISR_LARGE_RANGE_ACCEL
void isr_large_range_accel();
#endif*/
#ifdef ISR_BUTTON
void isr_button();
#endif
/*#ifdef ISR_SPIRX
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
#endif*/
#ifdef TASK_APPLICATION
void task_application();
typedef void (task_application_fn)(uint16_t);

bool scheduler_register_application_task(task_application_fn*, uint16_t period, bool continuous);
bool scheduler_free_application_task(task_application_fn*);
#endif

#endif