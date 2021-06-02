#ifndef __LW_SW_UART_H__
#define __LW_SW_UART_H__

#include <mbed.h>

#ifdef LN_SW_UART_TX_INVERTED  //normally output is driven via NPN, so it is inverted
        #ifndef LN_SW_UART_SET_TX_LOW // putting a 1 to the pin to switch on NPN transistor
        #define LN_SW_UART_SET_TX_LOW()  txPin->write(1)   // to pull down LN line to drive low level
        #endif

        #ifndef LN_SW_UART_SET_TX_HIGH                              // putting a 0 to the pin to switch off NPN transistor
        #define LN_SW_UART_SET_TX_HIGH() txPin->write(0) // master pull up will take care of high LN level
        #endif
#else //non-inverted output logic
        #ifndef LN_SW_UART_SET_TX_LOW                               // putting a 1 to the pin to switch on NPN transistor
        #define LN_SW_UART_SET_TX_LOW() txPin->write(0)  // to pull down LN line to drive low level
        #endif
        #ifndef LN_SW_UART_SET_TX_HIGH                              // putting a 0 to the pin to switch off NPN transistor
        #define LN_SW_UART_SET_TX_HIGH() txPin->write(1) // master pull up will take care of high LN level
        #endif
#endif // LN_SW_UART_TX_INVERTED

#ifdef LN_SW_UART_TX_INVERTED
        #ifdef LN_SW_UART_RX_INVERTED
        #define IS_LN_COLLISION()       txPin->read() != rxPin->read()
        #else
        #define IS_LN_COLLISION()       txPin->read() == rxPin->read()
        #endif
#else
        #ifdef LN_SW_UART_RX_INVERTED
        #define IS_LN_COLLISION()       txPin->read() == rxPin->read()
        #else
        #define IS_LN_COLLISION()       txPin->read() != rxPin->read()
        #endif
#endif


#define LN_ST_IDLE            0   // net is free for anyone to start transmission
#define LN_ST_CD_BACKOFF      1   // timer interrupt is counting backoff bits
#define LN_ST_TX_COLLISION    2   // just sending break after creating a collision
#define LN_ST_TX              3   // transmitting a packet
#define LN_ST_RX              4   // receiving bytes

#define LN_COLLISION_TICKS 15
#define LN_TX_RETRIES_MAX  25

          // The Start Bit period is a full bit period + half of the next bit period
          // so that the bit is sampled in middle of the bit
const std::chrono::microseconds LN_BIT_PERIOD             = 60us;
const std::chrono::microseconds LN_TIMER_RX_START_PERIOD  = LN_BIT_PERIOD + (LN_BIT_PERIOD / 2);
const std::chrono::microseconds LN_TIMER_RX_RELOAD_PERIOD = LN_BIT_PERIOD;
const std::chrono::microseconds LN_TIMER_TX_RELOAD_PERIOD = LN_BIT_PERIOD;
//  14,4 us delay borrowed from FREDI sysdef.h
const std::chrono::microseconds LN_TIMER_TX_RELOAD_ADJUSTED = 59us;
// ATTENTION !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// LN_TIMER_TX_RELOAD_ADJUST is a value for an error correction. This is needed for
// every start of a byte. The first bit is to long. Therefore we need to reduce the
// reload value of the bittimer.
// The following value depences highly on used compiler, optimizationlevel and hardware.
// Define the value in sysdef.h. This is very project specific.
// For the FREDI hard- and software it is nearly a quarter of a LN_BIT_PERIOD.
// Olaf Funke, 19th October 2007
// ATTENTION !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#ifndef LN_TIMER_TX_RELOAD_ADJUST
#define LN_TIMER_TX_RELOAD_ADJUST   0
// #error detect value by oszilloscope
#endif




// ATTENTION !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void initLocoNetHardware( LnBuf *RxBuffer, DigitalOut *txPin, InterruptIn *rxPin);
LN_STATUS sendLocoNetPacketTry(lnMsg *TxData, unsigned char ucPrioDelay);

#endif // __LW_SW_UART_H__
