#include "LocoNetFastClockClass.h"

#define FC_FLAG_DCS100_COMPATIBLE_SPEED 0x01
#define FC_FLAG_MINUTE_ROLLOVER_SYNC    0x02
#define FC_FLAG_NOTIFY_FRAC_MINS_TICK   0x04
#define FC_FRAC_MIN_BASE                                0x3FFF
#define FC_FRAC_RESET_HIGH                              0x78
#define FC_FRAC_RESET_LOW                               0x6D
#define FC_TIMER_TICKS                          65        // 65ms ticks
#define FC_TIMER_TICKS_REQ                              250        // 250ms waiting for Response to FC Req

LocoNetFastClockClass::LocoNetFastClockClass() {
  init(0, 0, 0);
}

void LocoNetFastClockClass::init(uint8_t DCS100CompatibleSpeed, uint8_t CorrectDCS100Clock, uint8_t NotifyFracMin)
{
  fcState = FC_ST_IDLE ;
  
  fcFlags = 0;
  if(DCS100CompatibleSpeed)
        fcFlags |= FC_FLAG_DCS100_COMPATIBLE_SPEED ;

  if(CorrectDCS100Clock)
        fcFlags |= FC_FLAG_MINUTE_ROLLOVER_SYNC ;

  if(NotifyFracMin)
        fcFlags |= FC_FLAG_NOTIFY_FRAC_MINS_TICK ;
}

void LocoNetFastClockClass::poll(void)
{
  LocoNet.send( OPC_RQ_SL_DATA, FC_SLOT, 0 ) ;
}

void LocoNetFastClockClass::doNotify( uint8_t Sync )
{
  if(notifyFastClock)
        notifyFastClock(fcSlotData.clk_rate, fcSlotData.days,
          (fcSlotData.hours_24 >= (128-24)) ? fcSlotData.hours_24 - (128-24) : fcSlotData.hours_24 % 24 ,
          fcSlotData.mins_60 - (127-60 ), Sync ) ;
}

void LocoNetFastClockClass::processMessage( lnMsg *LnPacket )
{
  if( ( LnPacket->fc.slot == FC_SLOT ) && ( ( LnPacket->fc.command == OPC_WR_SL_DATA ) || ( LnPacket->fc.command == OPC_SL_RD_DATA ) ) )
  {
    if( LnPacket->fc.clk_cntrl & 0x40 )
    {
      if( fcState >= FC_ST_REQ_TIME )
      {
                memcpy( &fcSlotData, &LnPacket->fc, sizeof( fastClockMsg ) ) ; 
                                
        doNotify( 1 ) ;

                if(notifyFastClockFracMins && fcFlags & FC_FLAG_NOTIFY_FRAC_MINS_TICK )
                  notifyFastClockFracMins( FC_FRAC_MIN_BASE - ( ( fcSlotData.frac_minsh << 7 ) + fcSlotData.frac_minsl ) );

        fcState = FC_ST_READY ;
      }
    }
    else
      fcState = FC_ST_DISABLED ;
  }
}

void LocoNetFastClockClass::process66msActions(void)
{
                // If we are all initialised and ready then increment accumulators
  if( fcState == FC_ST_READY )
  {
    fcSlotData.frac_minsl +=  fcSlotData.clk_rate ;
    if( fcSlotData.frac_minsl & 0x80 )
    {
      fcSlotData.frac_minsl &= ~0x80 ;

      fcSlotData.frac_minsh++ ;
      if( fcSlotData.frac_minsh & 0x80 )
      {
                                        // For the next cycle prime the fraction of a minute accumulators
        fcSlotData.frac_minsl = FC_FRAC_RESET_LOW ;
                                
                                        // If we are in FC_FLAG_DCS100_COMPATIBLE_SPEED mode we need to run faster
                                        // by reducong the FRAC_MINS duration count by 128
        fcSlotData.frac_minsh = FC_FRAC_RESET_HIGH + (fcFlags & FC_FLAG_DCS100_COMPATIBLE_SPEED) ;

        fcSlotData.mins_60++;
        if( fcSlotData.mins_60 >= 0x7F )
        {
          fcSlotData.mins_60 = 127 - 60 ;

          fcSlotData.hours_24++ ;
          if( fcSlotData.hours_24 & 0x80 )
          {
            fcSlotData.hours_24 = 128 - 24 ;

            fcSlotData.days++;
          }
        }


                        // We either send a message out onto the LocoNet to change the time,
                        // which we will also see and act on or just notify our user
                        // function that our internal time has changed.
                if( fcFlags & FC_FLAG_MINUTE_ROLLOVER_SYNC )
                {
                        fcSlotData.command = OPC_WR_SL_DATA ;
                        LocoNet.send((lnMsg*)&fcSlotData) ;
                }
                else
                        doNotify( 0 ) ;
      }
    }

        if( notifyFastClockFracMins && (fcFlags & FC_FLAG_NOTIFY_FRAC_MINS_TICK ))
          notifyFastClockFracMins( FC_FRAC_MIN_BASE - ( ( fcSlotData.frac_minsh << 7 ) + fcSlotData.frac_minsl ) ) ;
  }

  if( fcState == FC_ST_IDLE )
  {
    LocoNet.send( OPC_RQ_SL_DATA, FC_SLOT, 0 ) ;
    fcState = FC_ST_REQ_TIME ;
  }
}
