#ifndef __LOCONET_FAST_CLOCK_H__
#define __LOCONET_FAST_CLOCK_H__

#include "LocoNet.h"

/************************************************************************************
        The LocoNet fast clock in the Command Station is driven from a 65.535 ms
    time base. A normal minute takes approximately 915 x 65.535 ms ticks.

        The LocoNet fast clock values are stored in a special slot in the Command
        Station called the fast clock slot which is slot number 0x7B or 123
        
        Each of the fields in the slot are supposed to count up until the most significant bit
        is 0x80 and then rollover the appropriate values and reset however this behaviour
        does not seem to hold for all fields and so some corrction factors are needed

        An important part of syncing to the Fast Clock master is to interpret the current
        FRAC_MINS fields so that a Fast Clock Slave can sync to the part minute and then
        rollover it's accumulators in sync with the master. The FRAC_MINS counter is a
        14 bit counter that is stored in the two 7 bit FRAC_MINSL & FRAC_MINSH fields.
        It counts up the FRAC_MINSL field until it rolls over to 0x80 and then increments
        the FRAC_MINSH high field until it rolls over to 0x80 and then increments the minute,
        hour and day fields as appropriate and then resets the FRAC_MINS fields to 0x4000 - 915
        which is stored in each of the 7 bit fields.
 
        HOWEVER when the DCS100 resets FRAC_MINS fields to 0x4000 - 915, it then immediately
        rolls over a 128 count and so the minute is short by 915 - 128 65.535 ms ticks, so it
        runs too fast. To correct this problem the fast clock slot can be overwritten with
        corrected FRAC_MINS field values that the DCS100 will then increment correctly.

        This implementation of a LocoNet Fast Clock Slave has two features to correct these
        short commings:
        
        A) It has the option to reduce the FRAC_MINS count by 128 so that it keeps in step with
        the DCS100 Fast Clock which normally runs too fast. This is enabled by passing in the
        FC_FLAG_DCS100_COMPATIBLE_SPEED flag bit to the init() function. 
        
        B) It has the option to overwrite the LocoNet Fast Clock Master slot values with corrected
        FRAC_MINS fields imediately after it rolls-over the fast minute, to make the DCS100 not run
        too fast as it normally does.   
        
        There also seems to be problems with the hours field not rolling over correctly from 23
        back to 0 and so there is extra processing to work out the hours when it has rolled over
        to 0x80 or 0x00 by the time the bit 7 is cleared. This seems to cause the DT400 throttle
        problems as well and so when running in FC_FLAG_MINUTE_ROLLOVER_SYNC mode, this should
        be corrected. 

        The DT400 throttle display seems to decode the minutes incorrectly by 1 count and so we
        have to make the same interpretation here which is why there is a 127 and not a 128
    roll-over for the minutes. 
***********************************************************************************************/

typedef enum
{
  FC_ST_IDLE,
  FC_ST_REQ_TIME,
  FC_ST_READY,
  FC_ST_DISABLED,
} FC_STATE ;

class LocoNetFastClockClass
{
  private:
        FC_STATE                fcState ;                       // State of the Fast Clock Slave 
        uint8_t                 fcFlags ;                       // Storage of the option flags passed into initFastClock()
        fastClockMsg    fcSlotData ;            // Primary storage for the Fast Clock slot data 
        uint8_t                 fcLastPeriod ;          // Period of last tick so we can alternate between
        
        void doNotify( uint8_t Sync );

  public:
        void init(uint8_t DCS100CompatibleSpeed, uint8_t CorrectDCS100Clock, uint8_t NotifyFracMin);
        void poll(void);
        void processMessage(lnMsg *LnPacket );
        void process66msActions(void);
};

#if defined (__cplusplus)
        extern "C" {
#endif

// FastClock notify Call-back functions
extern void notifyFastClock( uint8_t Rate, uint8_t Day, uint8_t Hour, uint8_t Minute, uint8_t Sync ) __attribute__ ((weak));
extern void notifyFastClockFracMins( uint16_t FracMins ) __attribute__ ((weak));

#if defined (__cplusplus)
}
#endif

	  
#endif // __LOCONET_FAST_CLOCK_H__
