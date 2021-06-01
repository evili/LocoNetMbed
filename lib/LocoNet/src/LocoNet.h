#ifndef __LOCONET_H__
#define __LOCONET_H__

// CD Backoff starts after the Stop Bit (Bit 9) and has a minimum or 20 Bit Times
// but initially starts with an additional 20 Bit Times 
#define   LN_CARRIER_TICKS      20  // carrier detect backoff - all devices have to wait this
#define   LN_MASTER_DELAY        6  // non master devices have to wait this additionally
#define   LN_INITIAL_PRIO_DELAY 20  // initial attempt adds priority delay
#define   LN_BACKOFF_MIN      (LN_CARRIER_TICKS + LN_MASTER_DELAY)      // not going below this
#define   LN_BACKOFF_INITIAL  (LN_BACKOFF_MIN + LN_INITIAL_PRIO_DELAY)  // for the first normal tx attempt
#define   LN_BACKOFF_MAX      (LN_BACKOFF_INITIAL + 10)                 // lower priority is not supported

#include "ln_opc.h"
#include "ln_buf.h"

#include "LocoNetClass.h"
#include "LocoNetThrottleClass.h"
#include "LocoNetFastClockClass.h"
#include "LocoNetSystemVariableClass.h"
#include "LocoNetCVClass.h"

/************************************************************************************
    Call-back functions
************************************************************************************/

#if defined (__cplusplus)
        extern "C" {
#endif

extern void notifySensor( uint16_t Address, uint8_t State ) __attribute__ ((weak));

// Address: Switch Address.
// Output: Value 0 for Coil Off, anything else for Coil On
// Direction: Value 0 for Closed/GREEN, anything else for Thrown/RED
// state: Value 0 for no input, anything else for activated
// Sensor: Value 0 for 'Aux'/'thrown' anything else for 'switch'/'closed'
extern void notifySwitchRequest( uint16_t Address, uint8_t Output, uint8_t Direction ) __attribute__ ((weak));
extern void notifySwitchReport( uint16_t Address, uint8_t Output, uint8_t Direction ) __attribute__ ((weak));
extern void notifySwitchOutputsReport( uint16_t Address, uint8_t ClosedOutput, uint8_t ThrownOutput ) __attribute__ ((weak));
extern void notifySwitchState( uint16_t Address, uint8_t Output, uint8_t Direction ) __attribute__ ((weak));
extern void notifyPower( uint8_t State ) __attribute__ ((weak));
extern void notifyLongAck(uint8_t d1, uint8_t d2) __attribute__ ((weak));  

// Power management, Transponding and Multi-Sense Device info Call-back functions
extern void notifyMultiSenseTransponder( uint16_t Address, uint8_t Zone, uint16_t LocoAddress, uint8_t Present ) __attribute__ ((weak));
extern void notifyMultiSensePower( uint8_t BoardID, uint8_t Subdistrict, uint8_t Mode, uint8_t Direction ) __attribute__ ((weak));

#if defined (__cplusplus)
}
#endif



extern LocoNetClass LocoNet;

#endif // __LOCONET_H__
