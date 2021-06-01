#ifndef _LN_CONFIG_H_INCLUDED
#define _LN_CONFIG_H_INCLUDED

/*
 ****************************************************************************
 * Copyright (C) 2004 Alex Shepherd
 * 
 * Portions Copyright (C) Digitrax Inc.
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
 ****************************************************************************
 * 2014/10/10 - Tom Knox
 *
 * Changes made to allow library to run with Mega 2560 (and similar) processors
 *
 * To differentiate between UNO and the 2560 I added #ifdef-s with defines
 * the 2560 has that UNO doesn't.  These are all commented with my initials (twk)
 *
 * Thanks to the neatness and modularity of Alex, all the changes are isolated
 * to just the ln_config.h (this) file!
 *
 ****************************************************************************
 * 2014/11/10 - John Plocher
 *
 * refactored to put all the MEGA/UNO changes together in one conditional block
 * to make it easier to port to other chipsets
 *
 ****************************************************************************
 * 2020/03/28 - Hans Tanner
 * added 2 new defines to ln_config.h see lines 70 and 71 below
 * it is now possible to set the logic of the Rx line as well, which is needed
 * when using an interface that uses the same (normally inverse) logic on both lines
 * added registers to support the edge change of the pin interrupt from falling to
 * rising depending on receiving logic
 * NOTE: THIS IS ONLY DONE FOR THE ARDUINO UNO SO FAR. NEEDS TO BE ADDED FOR THE DUE
 * AND THE ATTINY BOARDS IF YOU WANT TO WORK WITH THEM
 * changed the definition of the SET_TX macros in ln_sw_uart.h line 48 following to use
 * correct bit levels depending on inverse / non-inverse transmit. 
 */


// Uncomment the #define LN_SW_UART_TX_INVERTED below to Invert the polarity of the RX Signal
// Can be used with an interface with Serialport like behavior, Rx and Tx need to use the same logic level
// With the commonly used DIY LocoNet circuitry, its Not Normal to invert the RX Signal
//#define LN_SW_UART_RX_INVERTED

// Uncomment the #define LN_SW_UART_TX_INVERTED below to Invert the polarity of the TX Signal
// This is typically done where the output signal pin is connected to a NPN Transistor to pull-down the LocoNet
// which is commonly used in DIY circuit designs, so its normal to be Inverted 
#define LN_SW_UART_TX_INVERTED

#define LN_FREQUENCY              16667 // Hz
// #define LN_TMR_PRESCALER              1
// #define LN_TIMER_TX_RELOAD_ADJUST   106 //  14,4 us delay borrowed from FREDI sysdef.h
// #define LN_TX_RETRIES_MAX            25


// *****************************************************************************
// *                                                              Arduino MEGA *
// *                            The RX port *MUST BE* the ICP5 pin             *
// *                            (port PINL bit PL1, Arduino pin 48 on a 2560)  *
// *****************************************************************************
#endif	// include file
