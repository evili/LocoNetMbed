/****************************************************************************
 * Copyright (C) 2002 Alex Shepherd
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 *****************************************************************************
 * 
 * Title :   LocoNet Software UART Access library (ln_sw_uart.c)
 * Author:   Alex Shepherd <kiwi64ajs@sourceforge.net>
 * Date:     13-Aug-2002
 * Software:  AVR-GCC with AVR-AS
 * Target:    any AVR device
 * 
 * DESCRIPTION
 * Basic routines for interfacing to the LocoNet via any output pin and
 * either the Analog Comparator pins or the Input Capture pin
 * 
 * The receiver uses the Timer1 Input Capture Register and Interrupt to detect
 * the Start Bit and then the Compare A Register for timing the subsequest
 * bit times.
 * 
 * The Transmitter uses just the Compare A Register for timing all bit times
 * 
 * USAGE
 * See the C include ln_interface.h file for a description of each function
 * 
 *****************************************************************************/

#include "ln_config.h"
#include "LocoNet.h"
#include "ln_buf.h"    
#include "ln_sw_uart.h"    
#include <mbed.h>

volatile uint8_t  lnState ;
volatile uint8_t  lnBitCount ;
volatile uint8_t  lnCurrentByte ;
volatile uint16_t lnCompareTarget ;

LnBuf             * lnRxBuffer ;
volatile lnMsg    * volatile lnTxData ;
volatile uint8_t  lnTxIndex ;
volatile uint8_t  lnTxLength ;
volatile uint8_t  lnTxSuccess ;   // this boolean flag as a message from timer interrupt to send function

DigitalOut *txPin;
InterruptIn *rxPin;

DigitalOut lblue(LED2);
DigitalOut lred(LED3);

Ticker lnTicker;

void LN_TMR_SIGNAL();

inline void scheduleLnTicker(const std::chrono::microseconds period) {
  lnTicker.detach();
  lnTicker.attach(LN_TMR_SIGNAL, period);
}


/**************************************************************************
 *
 * Start Bit Interrupt Routine
 *
 * DESCRIPTION
 * This routine is executed when a falling edge on the incoming serial
 * signal is detected. It disables further interrupts and enables
 * timer interrupts (bit-timer) because the UART must now receive the
 * incoming data.
 *
 **************************************************************************/
void LN_SB_SIGNAL()
{
  lblue = !lblue;
  // Disable the Input Comparator Interrupt
  // TODO: cbi( LN_SB_INT_ENABLE_REG, LN_SB_INT_ENABLE_BIT );     
  rxPin->disable_irq();
  // Set the State to indicate that we have begun to Receive
  lnState = LN_ST_RX;
  // Get the Current Timer1 Count and Add the offset for the Compare target
  // TODO: lnCompareTarget = LN_TMR_INP_CAPT_REG + LN_TIMER_RX_START_PERIOD ;
  // LN_TMR_OUTP_CAPT_REG = lnCompareTarget ;

  // Clear the current Compare interrupt status bit and enable the Compare interrupt
  // TODO: sbi(LN_TMR_INT_STATUS_REG, LN_TMR_INT_STATUS_BIT) ;
  // TODO: sbi(LN_TMR_INT_ENABLE_REG, LN_TMR_INT_ENABLE_BIT) ; 
  scheduleLnTicker(LN_TIMER_RX_START_PERIOD);

  // Reset the bit counter so that on first increment it is on 0
  lnBitCount = 0;
}

/**************************************************************************
 *
 * Timer Tick Interrupt
 *
 * DESCRIPTION
 * This routine coordinates the transmition and reception of bits. This
 * routine is automatically executed at a rate equal to the baud-rate. When
 * transmitting, this routine shifts the bits and sends it. When receiving,
 * it samples the bit and shifts it into the buffer.
 *
 **************************************************************************/
void LN_TMR_SIGNAL()    /* signal handler for timer0 overflow */
{
  lred = !lred;
  // Advance the Compare Target by a bit period
  // TODO: lnCompareTarget += LN_TIMER_RX_RELOAD_PERIOD;
  // TODO: LN_TMR_OUTP_CAPT_REG = lnCompareTarget;
  // only need to adjust reload period if coming from RX start bit
  if ((lnBitCount==0) && (lnState == LN_ST_RX)) {
    scheduleLnTicker(LN_TIMER_RX_RELOAD_PERIOD);
  }

  lnBitCount++;                // Increment bit_counter

  if( lnState == LN_ST_RX ) {  // Are we in RX mode
    if( lnBitCount < 9)  {   // Are we in the Stop Bits phase
      lnCurrentByte >>= 1;
      if (rxPin->read() == LN_RX_HIGH) {
        lnCurrentByte |= 0x80;
      }
      return ;
    }

    // Clear the Start Bit Interrupt Status Flag and Enable ready to 
    // detect the next Start Bit
    // TODO: sbi( LN_SB_INT_STATUS_REG, LN_SB_INT_STATUS_BIT ) ;
    // TODO: sbi( LN_SB_INT_ENABLE_REG, LN_SB_INT_ENABLE_BIT ) ;
    rxPin->enable_irq();

    // If the Stop bit is not Set then we have a Framing Error
    if( rxPin->read() == LN_RX_LOW) {
      // ERROR_LED_ON();
      lnRxBuffer->Stats.RxErrors++ ;
    } 
    else { // Put the received byte in the buffer
      addByteLnBuf( lnRxBuffer, lnCurrentByte ) ;
    }
    lnBitCount = 0 ;
    lnState = LN_ST_CD_BACKOFF ;
  }

  if( lnState == LN_ST_TX ) {   // Are we in the TX State
    // To get to this point we have already begun the TX cycle so we need to 
    // first check for a Collision. 
    if ( IS_LN_COLLISION() ) {			 // Collision?
      lnBitCount = 0 ;
      lnState = LN_ST_TX_COLLISION ;
      // ERROR_LED_ON();
    } 
    else if( lnBitCount < 9) {   			 // Send each Bit
      if( lnCurrentByte & 0x01 ) {
        LN_SW_UART_SET_TX_HIGH();
      } 
      else {
        LN_SW_UART_SET_TX_LOW();
      }
      lnCurrentByte >>= 1;
    } 
    else if( lnBitCount ==  9) {   		 // Generate stop-bit
      LN_SW_UART_SET_TX_HIGH();
    } 
    else if( ++lnTxIndex < lnTxLength ) {  // Any more bytes in buffer
      // Setup for the next byte
      lnBitCount = 0 ;
      lnCurrentByte = lnTxData->data[ lnTxIndex ] ;

      // Begin the Start Bit
      LN_SW_UART_SET_TX_LOW();

      // Get the Current Timer1 Count and Add the offset for the Compare target
      // added adjustment value for bugfix (Olaf Funke)
      // lnCompareTarget = LN_TMR_COUNT_REG + LN_TIMER_TX_RELOAD_PERIOD - LN_TIMER_TX_RELOAD_ADJUST; 
      // LN_TMR_OUTP_CAPT_REG = lnCompareTarget ;
      // Needed??
      scheduleLnTicker(LN_TIMER_TX_RELOAD_ADJUSTED);
    } 
    else {
      // Successfully Sent all bytes in the buffer
      // so set the Packet Status to Done
      lnTxSuccess = 1 ;

      // Now copy the TX Packet into the RX Buffer
      addMsgLnBuf( lnRxBuffer, lnTxData );

      // Begin CD Backoff state
      lnBitCount = 0 ;
      lnState = LN_ST_CD_BACKOFF ;     
    }
  }

  // Note we may have got here from a failed TX cycle, if so BitCount will be 0
  if( lnState == LN_ST_TX_COLLISION ) {
    if( lnBitCount == 0 ) {
      // Pull the TX Line low to indicate Collision
      LN_SW_UART_SET_TX_LOW();
      // ERROR_LED_ON();
    } 
    else if( lnBitCount >= LN_COLLISION_TICKS ) {
      // Release the TX Line
      LN_SW_UART_SET_TX_HIGH();
      // ERROR_LED_OFF();

      lnBitCount = 0 ;
      lnState = LN_ST_CD_BACKOFF ;

      lnRxBuffer->Stats.Collisions++ ;
    }
  }

  if( lnState == LN_ST_CD_BACKOFF ) {
    if( lnBitCount == 0 ) {
      // Even though we are waiting, other nodes may try and transmit early
      // so Clear the Start Bit Interrupt Status Flag and Enable ready to 
      // detect the next Start Bit
      // TODO: sbi( LN_SB_INT_STATUS_REG, LN_SB_INT_STATUS_BIT ) ;
      // TODO: sbi( LN_SB_INT_ENABLE_REG, LN_SB_INT_ENABLE_BIT ) ;
      rxPin->enable_irq();
    } 
    else if( lnBitCount >= LN_BACKOFF_MAX ) { 
      // declare network to free after maximum backoff delay
      lnState = LN_ST_IDLE ;
      // TODO: cbi( LN_TMR_INT_ENABLE_REG, LN_TMR_INT_ENABLE_BIT ) ;
      lnTicker.detach();
    }
  }
}


void initLocoNetHardware( LnBuf *RxBuffer, DigitalOut *tx, InterruptIn *rx )
{
  lred = 0;
  lblue = 0;
  lnRxBuffer = RxBuffer ;
  txPin = tx;
  rxPin = rx;

  // Set the RX line to Input
  // TODO: cbi( LN_RX_DDR, LN_RX_BIT ) ;
  // Set the TX line to Inactive
  LN_SW_UART_SET_TX_HIGH();
  rxPin->disable_irq();
  #ifdef LN_SW_UART_RX_INVERTED
  rxPin->rise(LN_SB_SIGNAL);
  #else
  rxPin->fall(LN_SB_SIGNAL);
  #endif

  // First Enable the Analog Comparitor Power, 
  // Set the mode to Falling Edge
  // Enable Analog Comparator to Trigger the Input Capture unit
  // ACSR = (1<<ACI) | (1<<ACIS1) | (1<<ACIC) ;
  // Turn off the Analog Comparator
  // TODO: ACSR = 1<<ACD;
  // The noise canceler is enabled by setting the Input Capture Noise Canceler (ICNCn) bit in 
  // Timer/Counter Control Register B (TCCRnB). When enabled the noise canceler introduces addi- 
  // tional four system clock cycles of delay from a change applied to the input, to the update of the 
  // ICRn Register. The noise canceler uses the system clock and is therefore not affected by the 
  // prescaler.
  // TODO: TCCR1B |= (1<<ICNC1) ;    		// Enable Noise Canceler 
  lnState = LN_ST_IDLE ;

  rxPin->enable_irq();
  
  //Clear StartBit Interrupt flag
  // TODO: sbi( LN_SB_INT_STATUS_REG, LN_SB_INT_STATUS_BIT );
  //Enable StartBit Interrupt
  // TODO: sbi( LN_SB_INT_ENABLE_REG, LN_SB_INT_ENABLE_BIT );
  
  //Set rising edge for StartBit if signal is inverted
  // #ifdef LN_SW_UART_RX_INVERTED  
  // TODO: sbi(LN_SB_EDGE_CFG_REG, LN_SB_EDGE_BIT);
  // #endif
  
  // Set Timer Clock Source 
  // TODO: LN_TMR_CONTROL_REG = (LN_TMR_CONTROL_REG & 0xF8) | LN_TMR_PRESCALER;
}

LN_STATUS sendLocoNetPacketTry(lnMsg *TxData, unsigned char ucPrioDelay)
{
  uint8_t  CheckSum ;
  uint8_t  CheckLength ;

  lnTxLength = getLnMsgSize( TxData ) ;

  // First calculate the checksum as it may not have been done
  CheckLength = lnTxLength - 1 ;
  CheckSum = 0xFF ;

  for( lnTxIndex = 0; lnTxIndex < CheckLength; lnTxIndex++ ) {
    CheckSum ^= TxData->data[ lnTxIndex ] ;
  }

  TxData->data[ CheckLength ] = CheckSum ; 

  // clip maximum prio delay
  if (ucPrioDelay > LN_BACKOFF_MAX) {
    ucPrioDelay = LN_BACKOFF_MAX;
  }
  // if priority delay was waited now, declare net as free for this try
  CriticalSectionLock::enable();  // disabling interrupt to avoid confusion by ISR changing lnState while we want to do it
  if (lnState == LN_ST_CD_BACKOFF) {
    if (lnBitCount >= ucPrioDelay) {	// Likely we don't want to wait as long as
      lnState = LN_ST_IDLE;			// the timer ISR waits its maximum delay.
      
      // TODO: cbi( LN_TMR_INT_ENABLE_REG, LN_TMR_INT_ENABLE_BIT ) ;
      lnTicker.detach();
    }
  }
  CriticalSectionLock::disable();
  // sei();
  // a delayed start bit interrupt will happen now,
  // a delayed timer interrupt was stalled

  // If the Network is not Idle, don't start the packet
  if (lnState == LN_ST_CD_BACKOFF) {
    if (lnBitCount < LN_CARRIER_TICKS) {  // in carrier detect timer?
      return LN_CD_BACKOFF;
    } 
    else {
      return LN_PRIO_BACKOFF;
    }
  }

  if( lnState != LN_ST_IDLE ) {
    return LN_NETWORK_BUSY;  // neither idle nor backoff -> busy
  }
  // We need to do this with interrupts off.
  // The last time we check for free net until sending our start bit
  // must be as short as possible, not interrupted.
  // TODO: cli() ;
  CriticalSectionLock::enable();
  // Before we do anything else - Disable StartBit Interrupt
  // TODO: cbi( LN_SB_INT_ENABLE_REG, LN_SB_INT_ENABLE_BIT ) ;
  rxPin->disable_irq();

  if (rxPin->read() == LN_RX_HIGH) {
    // if (bit_is_set(LN_SB_INT_STATUS_REG, LN_SB_INT_STATUS_BIT)) {
    // first we disabled it, than before sending the start bit, we found out
    // that somebody was faster by examining the start bit interrupt request flag
    // TODO: sbi( LN_SB_INT_ENABLE_REG, LN_SB_INT_ENABLE_BIT ) ;
    // TODO: sei() ;  // receive now what our rival is sending
    rxPin->enable_irq();
    CriticalSectionLock::disable();
    return LN_NETWORK_BUSY;
  }

  LN_SW_UART_SET_TX_LOW();  // Begin the Start Bit

  // Get the Current Timer1 Count and Add the offset for the Compare target
  // added adjustment value for bugfix (Olaf Funke)
  // TODO: lnCompareTarget = LN_TMR_COUNT_REG + LN_TIMER_TX_RELOAD_PERIOD - LN_TIMER_TX_RELOAD_ADJUST;
  // TODO: LN_TMR_OUTP_CAPT_REG = lnCompareTarget ;

  // TODO: sei() ;  // Interrupts back on ...
  CriticalSectionLock::disable();

  lnTxData = TxData ;
  lnTxIndex = 0 ;
  lnTxSuccess = 0 ;

  // Load the first Byte
  lnCurrentByte = TxData->data[ 0 ] ;

  // Set the State to Transmit
  lnState = LN_ST_TX ;                      

  // Reset the bit counter
  lnBitCount = 0 ;                          

  // Clear the current Compare interrupt status bit and enable the Compare interrupt
  // TODO: sbi(LN_TMR_INT_STATUS_REG, LN_TMR_INT_STATUS_BIT) ;
  // TODO: sbi(LN_TMR_INT_ENABLE_REG, LN_TMR_INT_ENABLE_BIT) ;
  scheduleLnTicker(LN_TIMER_TX_RELOAD_ADJUSTED);

  while (lnState == LN_ST_TX) {
    // now busy wait until the interrupts do the rest
  }
  if (lnTxSuccess) {
    lnRxBuffer->Stats.TxPackets++ ;
    return LN_DONE;
  }
  if (lnState == LN_ST_TX_COLLISION) {
    return LN_COLLISION;
  }
  return LN_UNKNOWN_ERROR; // everything else is an error
}
