/**
\brief TelosB-specific definition of the "spi" bsp module.

\author Andrew Pullin <pullin@berkeley.edu>, January 2013.
*/

#include "p33fj128mc706a.h"
#include "bsp-spi.h" //OpenOS local SPI header
#include "spi.h"   //MicroChip SPI periphal header
#include "leds.h"
#include "ipspi1.h"
#include "utils.h"

//=========================== defines =========================================

//=========================== variables =======================================

typedef struct {
   // information about the current transaction
   uint8_t*        pNextTxByte;
   uint8_t         numTxedBytes;
   uint8_t         txBytesLeft;
   spi_return_t    returnType;
   uint8_t*        pNextRxByte;
   uint8_t         maxRxBytes;
   spi_first_t     isFirst;
   spi_last_t      isLast;
   // state of the module
   uint8_t         busy;
#ifdef SPI_IN_INTERRUPT_MODE
   // callback when module done
   spi_cbt         callback;
#endif
} spi_vars_t;

spi_vars_t spi_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

void spi_init() {
   // clear variables
   memset(&spi_vars,0,sizeof(spi_vars_t));

   ipspi1Config(); //From imageproc-lib
   //Config:
   //8-bit wide, MSD, 4 pin, master mode, SCK = 1.25Mhz

   // enable interrupts via the IEx SFRs
#ifdef SPI_IN_INTERRUPT_MODE
   IE1       |=  URXIE0;                         // we only enable the SPI RX interrupt
                                                 // since TX and RX happen concurrently,
                                                 // i.e. an RX completion necessarily
                                                 // implies a TX completion.
#endif
}

#ifdef SPI_IN_INTERRUPT_MODE
void spi_setCb(spi_cbt cb) {
   spi_vars.spi_cb = cb;
}
#endif

void spi_txrx(uint8_t*     bufTx,
              uint8_t      lenbufTx,
              spi_return_t returnType,
              uint8_t*     bufRx,
              uint8_t      maxLenBufRx,
              spi_first_t  isFirst,
              spi_last_t   isLast) {

#ifdef SPI_IN_INTERRUPT_MODE
   // disable interrupts
   __disable_interrupt();
#endif

   // register spi frame to send
   spi_vars.pNextTxByte      =  bufTx;
   spi_vars.numTxedBytes     =  0;
   spi_vars.txBytesLeft      =  lenbufTx;
   spi_vars.returnType       =  returnType;
   spi_vars.pNextRxByte      =  bufRx;
   spi_vars.maxRxBytes       =  maxLenBufRx;
   spi_vars.isFirst          =  isFirst;
   spi_vars.isLast           =  isLast;

   // SPI is now busy
   spi_vars.busy             =  1;

   // lower CS signal to have slave listening
   if (spi_vars.isFirst==SPI_FIRST) {
      ipspi1ChipSelect(FALSE);
   }

#ifdef SPI_IN_INTERRUPT_MODE
   // implementation 1. use a callback function when transaction finishes

   // write first byte to TX buffer
   U0TXBUF                   = *spi_vars.pNextTxByte;

   // re-enable interrupts
   __enable_interrupt();
#else
   // implementation 2. busy wait for each byte to be sent

   // send all bytes
   while (spi_vars.txBytesLeft>0) {
      // write next byte to TX buffer
      ipspi1PutByte(*(spi_vars.pNextTxByte));
      //ipspi1 function will busywait until TX done
        //TODO: Unclear if ipspi method results in byte readback into dsPIC?

       byte spiRX = SPI1BUF & 0xff;

      // save the byte just received in the RX buffer
      switch (spi_vars.returnType) {
         case SPI_FIRSTBYTE:
            if (spi_vars.numTxedBytes==0) {
               *spi_vars.pNextRxByte   = spiRX;
            }
            break;
         case SPI_BUFFER:
            *spi_vars.pNextRxByte      = spiRX;
            spi_vars.pNextRxByte++;
            break;
         case SPI_LASTBYTE:
            *spi_vars.pNextRxByte      = spiRX;
            break;
      }
      // one byte less to go
      spi_vars.pNextTxByte++;
      spi_vars.numTxedBytes++;
      spi_vars.txBytesLeft--;
   }

   // put CS signal high to signal end of transmission to slave
   if (spi_vars.isLast==SPI_LAST) {
      ipspi1ChipSelect(TRUE);
   }

   // SPI is not busy anymore
   spi_vars.busy             =  0;
#endif
}

//=========================== private =========================================

//=========================== interrupt handlers ==============================

kick_scheduler_t spi_isr() {
#ifdef SPI_IN_INTERRUPT_MODE
   // save the byte just received in the RX buffer
   switch (spi_vars.returnType) {
      case SPI_FIRSTBYTE:
         if (spi_vars.numTxedBytes==0) {
            *spi_vars.pNextRxByte   = U0RXBUF;
         }
         break;
      case SPI_BUFFER:
         *spi_vars.pNextRxByte      = U0RXBUF;
         spi_vars.pNextRxByte++;
         break;
      case SPI_LASTBYTE:
         *spi_vars.pNextRxByte      = U0RXBUF;
         break;
   }

   // one byte less to go
   spi_vars.pNextTxByte++;
   spi_vars.numTxedBytes++;
   spi_vars.txBytesLeft--;

   if (spi_vars.txBytesLeft>0) {
      // write next byte to TX buffer
      U0TXBUF                = *spi_vars.pNextTxByte;
   } else {
      // put CS signal high to signal end of transmission to slave
      if (spi_vars.isLast==SPI_LAST) {
         P4OUT                 |=  0x04;
      }
      // SPI is not busy anymore
      spi_vars.busy             =  0;

      // SPI is done!
      if (spi_vars.callback!=NULL) {
         // call the callback
         spi_vars.spi_cb();
         // kick the OS
         return KICK_SCHEDULER;
      }
   }
   return DO_NOT_KICK_SCHEDULER;
#else
   // this should never happpen!

   // we can not print from within the BSP. Instead:
   // blink the error LED
   leds_error_blink();
   // reset the board
   board_reset();

   return DO_NOT_KICK_SCHEDULER; // we will not get here, statement to please compiler
#endif
}
