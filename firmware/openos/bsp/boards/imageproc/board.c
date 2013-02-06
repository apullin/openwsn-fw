/**
\brief Imageproc2.4-specific definition of the "board" bsp module.

\author Andrew Pullin <pullin@berkeley.edu>, January 2013.
 */

#include "p33fj128mc706a.h"
#include "Generic.h"
#include "board.h"
// bsp modules
#include "leds.h"
#include "uart.h"
#include "bsp-spi.h"
#include "bsp_timer.h"
#include "../radio.h"
#include "radiotimer.h"
#include "debugpins.h"
//From imageproc-lib
#include "init_default.h"

//=========================== variables =======================================

//=========================== prototypes ======================================
extern kick_scheduler_t radiotimer_compare_isr(); //radiotimer.c
extern kick_scheduler_t radiotimer_overflow_isr();//radiotimer.c

//=========================== main ============================================

extern int mote_main(void);

int main(void) {
    return mote_main();
}

//=========================== public ==========================================

void board_init() {

    //Board init from imageproc-lib
    SetupClock();
    SwitchClocks();
    SetupPorts();

    int old_ipl;
    mSET_AND_SAVE_CPU_IP(old_ipl, 1)

    // initialize bsp modules
    debugpins_init();
    leds_init();
    //uart_init(); // not used on ImageProc2.4
    spi_init();
    bsp_timer_init();
    radio_init();
    radiotimer_init();

    // enable interrupts
    // TODO: how to do this on dsPIC33?
}

void board_sleep() {
    //True sleep on dsPIC33 is hard
    //Instead, CPU core is idled
    Idle(); //all clocks and periphs continue, exit from idle on any interrupt
}

void board_reset(){
    //dsPIC has an assembly instruction for CPU reset
    __asm__ volatile ("reset");
}

//=========================== private =========================================

//=========================== interrupt handlers ==============================

// DACDMA_VECTOR

// PORT2_VECTOR

//#pragma vector = USART1TX_VECTOR
//Note: On ImageProc2.4, UART1 is on the same pins as SPI1
/*
void __attribute__((interrupt, no_auto_psv)) _U1TXInterrupt(void) {
    debugpins_isr_set();
       if (uart_isr_tx()==1) {                       // UART; TX
    //      __bic_SR_register_on_exit(CPUOFF);
       }
    debugpins_isr_clr();
}

//#pragma vector = USART1RX_VECTOR
void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void) {
   debugpins_isr_set();
   if (uart_isr_rx()==1) {                       // UART: RX
//      __bic_SR_register_on_exit(CPUOFF);
   }
   debugpins_isr_clr();
}
*/

// PORT1_VECTOR

// TIMER1_VECTOR
void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
   debugpins_isr_set();
   if (bsp_timer_isr()==1) {                     // timer: 0
//      __bic_SR_register_on_exit(CPUOFF);
   }
   debugpins_isr_clr();
}

// ADC12_VECTOR

// SPI1 VECTOR
void __attribute__((__interrupt__, no_auto_psv)) _SPI1Interrupt(void)
{
    debugpins_isr_set();
    if (spi_isr()==1) {                           // SPI
      //TODO: CPU is awake on any interrupt for dsPIC
   }
   debugpins_isr_clr();
    IFS0bits.SPI1IF = 0;
}

// CAN1 interrupt, repurposed for software interrupt
void __attribute__((interrupt, no_auto_psv)) _C1Interrupt(void) {
    debugpins_isr_set();
    //TODO: CPU is awake on any interrupt for dsPIC
    debugpins_isr_clr();
}
//#pragma vector = COMPARATORA_VECTOR
//__interrupt void COMPARATORA_ISR (void) {
//   debugpins_isr_set();
//   __bic_SR_register_on_exit(CPUOFF);            // restart CPU
//   debugpins_isr_clr();
//}

//TIMER2_VECTOR
void __attribute__((interrupt, no_auto_psv)) _T2Interrupt(void) {
    debugpins_isr_set();
    if (radiotimer_overflow_isr() == 1) { // radiotimer
        //TODO: CPU is awake on any interrupt for dsPIC
    }
    debugpins_isr_clr();
}

// TIMERB0_VECTOR

// NMI_VECTOR


void __attribute__((interrupt, no_auto_psv)) _OC1Interrupt(void) {
    debugpins_isr_set();
    if (radiotimer_compare_isr() == 1) { // radiotimer
        //TODO: CPU is awake on any interrupt for dsPIC
    }
    debugpins_isr_clr();
}