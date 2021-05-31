#include "LocoNetThrottleClass.h"

// To make it easier to handle the Speed steps 0 = Stop, 1 = Em Stop and 2 -127
// normal speed steps we will swap speed steps 0 and 1 so that the normal
// range for speed steps is Stop = 1 2-127 is normal as before and now 0 = EmStop
static uint8_t SwapSpeedZeroAndEmStop( uint8_t Speed )
{
  if( Speed == 0 )
    return 1 ;

  if( Speed == 1 )
    return 0 ;

  return Speed ;
}

void LocoNetThrottleClass::updateAddress(uint16_t Address, uint8_t ForceNotify )
{
  if( ForceNotify || myAddress != Address )
  {
    myAddress = Address ;
    if(notifyThrottleAddress)
                notifyThrottleAddress( myUserData, myState, Address, mySlot ) ;
  }
}

void LocoNetThrottleClass::updateSpeed(uint8_t Speed, uint8_t ForceNotify )
{
  if( ForceNotify || mySpeed != Speed )
  {
    mySpeed = Speed ;
        if(notifyThrottleSpeed)
                notifyThrottleSpeed( myUserData, myState, SwapSpeedZeroAndEmStop( Speed ) ) ;
  }
}

void LocoNetThrottleClass::updateState(TH_STATE State, uint8_t ForceNotify )
{
  TH_STATE  PrevState ;

  if( ForceNotify || myState != State )
  {
    PrevState = myState ;
    myState = State ;
        if(notifyThrottleState)
                notifyThrottleState( myUserData, PrevState, State ) ;
  }
}

void LocoNetThrottleClass::updateStatus1(uint8_t Status, uint8_t ForceNotify )
{
  register uint8_t Mask ;       // Temporary uint8_t Variable for bitwise AND to force
  // the compiler to only do 8 bit operations not 16

  if( ForceNotify || myStatus1 != Status )
  {
    myStatus1 = Status ;
        if(notifyThrottleSlotStatus)
                notifyThrottleSlotStatus( myUserData, Status ) ;

    Mask = LOCO_IN_USE ;
    updateState( ( ( Status & Mask ) == Mask ) ? TH_ST_IN_USE : TH_ST_FREE, ForceNotify ) ;
  }
}

void LocoNetThrottleClass::updateDirectionAndFunctions(uint8_t DirFunc0to4, uint8_t ForceNotify )
{
  uint8_t Diffs ;
  uint8_t Function ;
  uint8_t Mask ;

  if( ForceNotify || myDirFunc0to4 != DirFunc0to4 )
  {
    Diffs = myDirFunc0to4 ^ DirFunc0to4 ;
    myDirFunc0to4 = DirFunc0to4 ;

    // Check Functions 1-4
    for( Function = 1, Mask = 1; Function <= 4; Function++ )
    {
      if( notifyThrottleFunction && (ForceNotify || Diffs & Mask ))
        notifyThrottleFunction( myUserData, Function, DirFunc0to4 & Mask ) ;

      Mask <<= 1 ;
    }
    // Check Functions 0
    if( notifyThrottleFunction && ( ForceNotify || Diffs & DIRF_F0 ))
      notifyThrottleFunction( myUserData, 0, DirFunc0to4 & (uint8_t)DIRF_F0 ) ;

    // Check Direction
    if( notifyThrottleDirection && (ForceNotify || Diffs & DIRF_DIR ))
      notifyThrottleDirection( myUserData, myState, DirFunc0to4 & (uint8_t)DIRF_DIR ) ;
  }
}

void LocoNetThrottleClass::updateFunctions5to8(uint8_t Func5to8, uint8_t ForceNotify )
{
  uint8_t Diffs ;
  uint8_t Function ;
  uint8_t Mask ;

  if( ForceNotify || myFunc5to8 != Func5to8 )
  {
    Diffs = myFunc5to8 ^ Func5to8 ;
    myFunc5to8 = Func5to8 ;

    // Check Functions 5-8
    for( Function = 5, Mask = 1; Function <= 8; Function++ )
    {
      if( notifyThrottleFunction && (ForceNotify || Diffs & Mask ))
        notifyThrottleFunction( myUserData, Function, Func5to8 & Mask ) ;

      Mask <<= 1 ;
    }
  }
}

void LocoNetThrottleClass::updateSpeedSteps(TH_SPEED_STEPS SpeedSteps, uint8_t ForceNotify)
{
  if( ForceNotify || ((myStatus1 & 0x07) != SpeedSteps ))
  {
        myStatus1 = (myStatus1 & 0xF8) | SpeedSteps;
        if(notifyThrottleSpeedSteps)
          notifyThrottleSpeedSteps(myUserData, SpeedSteps);
  }
}

#define SLOT_REFRESH_TICKS              600   // 600 * 100ms = 60 seconds between speed refresh

void LocoNetThrottleClass::process100msActions(void)
{
  if( myState == TH_ST_IN_USE )
  {
        myTicksSinceLastAction++ ;

        if( ( myDeferredSpeed ) || ( myTicksSinceLastAction > SLOT_REFRESH_TICKS ) )
        {
          LocoNet.send( OPC_LOCO_SPD, mySlot, ( myDeferredSpeed ) ? myDeferredSpeed : mySpeed ) ;

          if( myDeferredSpeed )
                myDeferredSpeed = 0 ;

          myTicksSinceLastAction = 0 ;
        }
  }
}

void LocoNetThrottleClass::init(uint8_t UserData, uint8_t Options, uint16_t ThrottleId )
{
  myState = TH_ST_FREE ;
  myThrottleId = ThrottleId ;
  myDeferredSpeed = 0 ;
  myUserData = UserData ;
  myOptions = Options ;
}


void LocoNetThrottleClass::processMessage(lnMsg *LnPacket )
{
  uint8_t  Data2 ;
  uint16_t  SlotAddress ;

  // Update our copy of slot information if applicable
  if( LnPacket->sd.command == OPC_SL_RD_DATA )
  {
    SlotAddress = (uint16_t) (( LnPacket->sd.adr2 << 7 ) + LnPacket->sd.adr ) ;

    if( mySlot == LnPacket->sd.slot )
    {
      // Make sure that the slot address matches even though we have the right slot number
      // as it is possible that another throttle got in before us and took our slot.
      if( myAddress == SlotAddress )
      {
        if(     ( myState == TH_ST_SLOT_RESUME ) && ( myThrottleId != (uint16_t)( ( LnPacket->sd.id2 << 7 ) + LnPacket->sd.id1 ) ) )
        {
          updateState( TH_ST_FREE, 1 ) ;
          if(notifyThrottleError)
            notifyThrottleError( myUserData, TH_ER_NO_LOCO ) ;
        }
        else
        {
          updateState( TH_ST_IN_USE, 1 ) ;
          updateAddress( SlotAddress, 1 ) ;
          updateSpeed( LnPacket->sd.spd, 1 ) ;
          updateDirectionAndFunctions( LnPacket->sd.dirf, 1 ) ;
          updateFunctions5to8( LnPacket->sd.snd, 1 ) ;

                  updateSpeedSteps(mySpeedSteps, 1);

            // We need to force a State update to cause a display refresh once all data is known
          updateState( TH_ST_IN_USE, 1 ) ;

            // Now Write our own Speed Steps and Throttle Id to the slot and write it back to the command station
          LnPacket->sd.command = OPC_WR_SL_DATA ;
          LnPacket->sd.stat = ( LnPacket->sd.stat & 0xf8) | mySpeedSteps;
          updateStatus1( LnPacket->sd.stat, 1 ) ;

          LnPacket->sd.id1 = (uint8_t) ( myThrottleId & 0x7F ) ;
          LnPacket->sd.id2 = (uint8_t) ( myThrottleId >> 7 );
                
          LocoNet.send( LnPacket ) ;
        }
      }
      // Ok another throttle did a NULL MOVE with the same slot before we did
      // so we have to try again
      else if( myState == TH_ST_SLOT_MOVE )
      {
        updateState( TH_ST_SELECT, 1 ) ;
        LocoNet.send( OPC_LOCO_ADR, (uint8_t) ( myAddress >> 7 ), (uint8_t) ( myAddress & 0x7F ) ) ;
      }
    }
    // Slot data is not for one of our slots so check if we have requested a new addres
    else
    {
      if( myAddress == SlotAddress )
      {
        if( ( myState == TH_ST_SELECT ) || ( myState == TH_ST_DISPATCH ) )
        {
          if( ( LnPacket->sd.stat & STAT1_SL_CONUP ) == 0 &&
            ( LnPacket->sd.stat & LOCO_IN_USE ) != LOCO_IN_USE )
          {
            if( myState == TH_ST_SELECT )
            {
              updateState( TH_ST_SLOT_MOVE, 1 ) ;
              mySlot = LnPacket->sd.slot ;
              Data2 = LnPacket->sd.slot ;
            }
            else
            {
              updateState( TH_ST_FREE, 1 ) ;
              Data2 = 0 ;
            }

            LocoNet.send( OPC_MOVE_SLOTS, LnPacket->sd.slot, Data2 ) ;
          }
          else
          {
            if(notifyThrottleError)
                                notifyThrottleError( myUserData, TH_ER_SLOT_IN_USE ) ;
            updateState( TH_ST_FREE, 1 ) ;
          }
        }
        else if( myState == TH_ST_SLOT_STEAL )
        {
                // Make Sure the Slot is actually IN_USE already as we are not going to do an SLOT_MOVE etc
          if( ( LnPacket->sd.stat & STAT1_SL_CONUP ) == 0 &&
            ( LnPacket->sd.stat & LOCO_IN_USE ) == LOCO_IN_USE )
          {  
                        mySlot = LnPacket->sd.slot ;

                        updateState( TH_ST_IN_USE, 1 ) ;

                        updateAddress( SlotAddress, 1 ) ;
                        updateSpeed( LnPacket->sd.spd, 1 ) ;
                        updateDirectionAndFunctions( LnPacket->sd.dirf, 1 ) ;
                        updateFunctions5to8( LnPacket->sd.snd, 1 ) ;
                        updateStatus1( LnPacket->sd.stat, 1 ) ;

                          // We need to force a State update to cause a display refresh once all data is known
                        updateState( TH_ST_IN_USE, 1 ) ;
                  }
          else
          {
            if(notifyThrottleError)
                                notifyThrottleError( myUserData, TH_ER_NO_LOCO ) ;
            updateState( TH_ST_FREE, 1 ) ;
          }
        }
        else if( myState == TH_ST_SLOT_FORCE_FREE )
        {
          LocoNet.send( OPC_SLOT_STAT1, LnPacket->sd.slot, (uint8_t) ( myStatus1 & ~(STAT1_SL_BUSY|STAT1_SL_ACTIVE) ) ) ;
          mySlot = 0xFF ;
          updateState( TH_ST_FREE, 1 ) ;
        }
      }

      if( myState == TH_ST_ACQUIRE )
      {
        mySlot = LnPacket->sd.slot ;
        updateState( TH_ST_IN_USE, 1 ) ;

        updateAddress( SlotAddress, 1 ) ;
        updateSpeed( LnPacket->sd.spd, 1 ) ;
        updateDirectionAndFunctions( LnPacket->sd.dirf, 1 ) ;
        updateStatus1( LnPacket->sd.stat, 1 ) ;
      }
    }
  }

  else if( ( ( LnPacket->sd.command >= OPC_LOCO_SPD ) && ( LnPacket->sd.command <= OPC_LOCO_SND ) ) ||
    ( LnPacket->sd.command == OPC_SLOT_STAT1 ) )
  {
    if( mySlot == LnPacket->ld.slot )
    {
      if( LnPacket->ld.command == OPC_LOCO_SPD )
        updateSpeed( LnPacket->ld.data, 0 ) ;

      else if( LnPacket->ld.command == OPC_LOCO_DIRF )
        updateDirectionAndFunctions( LnPacket->ld.data, 0 ) ;

      else if( LnPacket->ld.command == OPC_LOCO_SND )
        updateFunctions5to8( LnPacket->ld.data, 0 ) ;

      else if( LnPacket->ld.command == OPC_SLOT_STAT1 )
        updateStatus1( LnPacket->ld.data, 0 ) ;
    }
  }
  else if( LnPacket->lack.command == OPC_LONG_ACK )
  {
    if( ( myState >= TH_ST_ACQUIRE ) && ( myState <= TH_ST_SLOT_MOVE ) )
    {
      if( LnPacket->lack.opcode == ( OPC_MOVE_SLOTS & 0x7F ) )
        if(notifyThrottleError)
                        notifyThrottleError( myUserData, TH_ER_NO_LOCO ) ;

      if( LnPacket->lack.opcode == ( OPC_LOCO_ADR & 0x7F ) )
        if( notifyThrottleError)
                        notifyThrottleError( myUserData, TH_ER_NO_SLOTS ) ;

      updateState( TH_ST_FREE, 1 ) ;
    }
  }
}

uint16_t LocoNetThrottleClass::getAddress(void)
{
  return myAddress ;
}

TH_ERROR LocoNetThrottleClass::stealAddress(uint16_t Address)
{
  if( myState <= TH_ST_RELEASE )
  {
    updateAddress( Address, 1 ) ;
    updateState( TH_ST_SLOT_STEAL, 1 ) ;

    LocoNet.send( OPC_LOCO_ADR, (uint8_t) ( Address >> 7 ), (uint8_t) ( Address & 0x7F ) ) ;
    return TH_ER_OK ;
  }

  if(notifyThrottleError)
        notifyThrottleError( myUserData, TH_ER_BUSY ) ;
  return TH_ER_BUSY ;
}


TH_ERROR LocoNetThrottleClass::setAddress(uint16_t Address )
{
  if( myState <= TH_ST_RELEASE )
  {
    updateAddress( Address, 1 ) ;
    updateState( TH_ST_SELECT, 1 ) ;

    LocoNet.send( OPC_LOCO_ADR, (uint8_t) ( Address >> 7 ), (uint8_t) ( Address & 0x7F ) ) ;
    return TH_ER_OK ;
  }

  if(notifyThrottleError)
        notifyThrottleError( myUserData, TH_ER_BUSY ) ;
  return TH_ER_BUSY ;
}

TH_ERROR LocoNetThrottleClass::resumeAddress(uint16_t Address, uint8_t LastSlot )
{
  if( myState <= TH_ST_RELEASE )
  {
    mySlot = LastSlot ;
    updateAddress( Address, 1 ) ;
    updateState( TH_ST_SLOT_RESUME, 1 ) ;

    LocoNet.send( OPC_RQ_SL_DATA, LastSlot, 0 ) ;
    return TH_ER_OK ;
  }

  if(notifyThrottleError)
        notifyThrottleError( myUserData, TH_ER_BUSY ) ;
  return TH_ER_BUSY ;
}

TH_ERROR LocoNetThrottleClass::freeAddressForce(uint16_t Address )
{
  if( myState <= TH_ST_RELEASE )
  {
    updateAddress( Address, 1 ) ;
    updateState( TH_ST_SLOT_FORCE_FREE, 1 ) ;

    LocoNet.send( OPC_LOCO_ADR, (uint8_t) ( Address >> 7 ), (uint8_t) ( Address & 0x7F ) ) ;
    return TH_ER_OK ;
  }

  if(notifyThrottleError)
        notifyThrottleError( myUserData, TH_ER_BUSY ) ;
  return TH_ER_BUSY ;
}

TH_ERROR LocoNetThrottleClass::dispatchAddress(void)
{
  if( myState == TH_ST_IN_USE )
  {
    updateState( TH_ST_FREE, 1 ) ;
    LocoNet.send( OPC_MOVE_SLOTS, mySlot, 0 ) ;
    return TH_ER_OK ;
  }

  if(notifyThrottleError)
        notifyThrottleError( myUserData, TH_ER_NOT_SELECTED ) ;
  return TH_ER_NOT_SELECTED ;
}

TH_ERROR LocoNetThrottleClass::dispatchAddress(uint16_t Address )
{
  if( myState <= TH_ST_RELEASE )
  {
    updateAddress( Address, 1 ) ;
    updateState( TH_ST_DISPATCH, 1 ) ;

    LocoNet.send( OPC_LOCO_ADR, (uint8_t) ( Address >> 7 ), (uint8_t) ( Address & 0x7F ) ) ;
    return TH_ER_OK ;
  }

  if(notifyThrottleError)
        notifyThrottleError( myUserData, TH_ER_BUSY ) ;
  return TH_ER_BUSY ;
}

TH_ERROR LocoNetThrottleClass::acquireAddress(void)
{
  if( myState <= TH_ST_RELEASE )
  {
    updateState( TH_ST_ACQUIRE, 1 ) ;

    LocoNet.send( OPC_MOVE_SLOTS, 0, 0 ) ;
    return TH_ER_OK ;
  }

  if(notifyThrottleError)
        notifyThrottleError( myUserData, TH_ER_BUSY ) ;
  return TH_ER_BUSY ;
}

TH_ERROR LocoNetThrottleClass::releaseAddress(void)
{
  if( myState == TH_ST_IN_USE )
  {
    LocoNet.send( OPC_SLOT_STAT1, mySlot, (uint8_t) ( myStatus1 & ~(STAT1_SL_BUSY) ) ) ;

    mySlot = 0xFF ;
    updateState( TH_ST_RELEASE, 1 ) ;
    return TH_ER_OK ;
  }

  if(notifyThrottleError)
        notifyThrottleError( myUserData, TH_ER_NOT_SELECTED ) ;
  return TH_ER_NOT_SELECTED;
}

TH_ERROR LocoNetThrottleClass::idleAddress(void)
{
  if( myState == TH_ST_IN_USE )
  {
    LocoNet.send( OPC_SLOT_STAT1, mySlot, (uint8_t) ( myStatus1 & ~(STAT1_SL_ACTIVE) ) ) ;

    mySlot = 0xFF ;
    updateState( TH_ST_IDLE, 1 ) ;
    return TH_ER_OK ;
  }

  if(notifyThrottleError)
        notifyThrottleError( myUserData, TH_ER_NOT_SELECTED ) ;
  return TH_ER_NOT_SELECTED;
}

TH_ERROR LocoNetThrottleClass::freeAddress(void)
{
  if( myState == TH_ST_IN_USE )
  {
    LocoNet.send( OPC_SLOT_STAT1, mySlot, (uint8_t) ( myStatus1 & ~(STAT1_SL_BUSY|STAT1_SL_ACTIVE) ) ) ;

    mySlot = 0xFF ;
    updateState( TH_ST_FREE, 1 ) ;
    return TH_ER_OK ;
  }

  if(notifyThrottleError)
        notifyThrottleError( myUserData, TH_ER_NOT_SELECTED ) ;
  return TH_ER_NOT_SELECTED;
}

uint8_t LocoNetThrottleClass::getSpeed(void)
{
  return SwapSpeedZeroAndEmStop( mySpeed ) ;
}

TH_ERROR LocoNetThrottleClass::setSpeed(uint8_t Speed )
{
  if( myState == TH_ST_IN_USE )
  {
    Speed = SwapSpeedZeroAndEmStop( Speed ) ;

    if( mySpeed != Speed )
    {
      // Always defer any speed other than stop or em stop
      if( (myOptions & TH_OP_DEFERRED_SPEED) &&
        ( ( Speed > 1 ) || (myTicksSinceLastAction == 0 ) ) )
        myDeferredSpeed = Speed ;
      else
      {
        LocoNet.send( OPC_LOCO_SPD, mySlot, Speed ) ;
        myTicksSinceLastAction = 0 ;
        myDeferredSpeed = 0 ;
      }
    }
    return TH_ER_OK ;
  }

  if(notifyThrottleError)
        notifyThrottleError( myUserData, TH_ER_NOT_SELECTED ) ;
  return TH_ER_NOT_SELECTED ;
}

uint8_t LocoNetThrottleClass::getDirection(void)
{
  return myDirFunc0to4 & (uint8_t)DIRF_DIR ;
}

TH_ERROR LocoNetThrottleClass::setDirection(uint8_t Direction )
{
  if( myState == TH_ST_IN_USE )
  {
    LocoNet.send( OPC_LOCO_DIRF, mySlot,
    ( Direction ) ? (uint8_t) ( myDirFunc0to4 | DIRF_DIR ) : (uint8_t) ( myDirFunc0to4 & ~DIRF_DIR ) ) ;

    myTicksSinceLastAction = 0 ;
    return TH_ER_OK ;
  }

  if(notifyThrottleError)
        notifyThrottleError( myUserData, TH_ER_NOT_SELECTED ) ;
  return TH_ER_NOT_SELECTED ;
}

uint8_t LocoNetThrottleClass::getFunction(uint8_t Function )
{
  uint8_t Mask ;

  if( Function <= 4 )
  {
    Mask = (uint8_t) (1 << ((Function) ? Function - 1 : 4 )) ;
    return myDirFunc0to4 & Mask ;
  }

  Mask = (uint8_t) (1 << (Function - 5)) ;
  return myFunc5to8 & Mask ;
}

TH_ERROR LocoNetThrottleClass::setFunction(uint8_t Function, uint8_t Value )
{
  uint8_t Mask ;
  uint8_t OpCode ;
  uint8_t Data ;

  if( myState == TH_ST_IN_USE )
  {
    if( Function <= 4 )
    {
      OpCode = OPC_LOCO_DIRF ;
      Data = myDirFunc0to4 ;
      Mask = (uint8_t)(1 << ((Function) ? Function - 1 : 4 )) ;
    }
    else
    {
      OpCode = OPC_LOCO_SND ;
      Data = myFunc5to8 ;
      Mask = (uint8_t)(1 << (Function - 5)) ;
    }

    if( Value )
      Data |= Mask ;
    else
      Data &= (uint8_t)~Mask ;

    LocoNet.send( OpCode, mySlot, Data ) ;

    myTicksSinceLastAction = 0 ;
    return TH_ER_OK ;
  }

  if(notifyThrottleError)
        notifyThrottleError( myUserData, TH_ER_NOT_SELECTED ) ;
  return TH_ER_NOT_SELECTED ;
}

TH_ERROR LocoNetThrottleClass::setDirFunc0to4Direct(uint8_t Value )
{
  if( myState == TH_ST_IN_USE )
  {
    LocoNet.send( OPC_LOCO_DIRF, mySlot, Value & 0x7F ) ;
    return TH_ER_OK ;
  }

  if(notifyThrottleError)
        notifyThrottleError( myUserData, TH_ER_NOT_SELECTED ) ;
  return TH_ER_NOT_SELECTED ;
}

TH_ERROR LocoNetThrottleClass::setFunc5to8Direct(uint8_t Value )
{
  if( myState == TH_ST_IN_USE )
  {
    LocoNet.send( OPC_LOCO_SND, mySlot, Value & 0x7F ) ;
    return TH_ER_OK ;
  }

  if(notifyThrottleError)
        notifyThrottleError( myUserData, TH_ER_NOT_SELECTED ) ;
  return TH_ER_NOT_SELECTED ;
}

TH_STATE LocoNetThrottleClass::getState(void)
{
  return myState ;
}


const char *LocoNetThrottleClass::getStateStr( TH_STATE State )
{
  switch( State )
  {
  case TH_ST_FREE:
    return "Free" ;

  case TH_ST_IDLE:
    return "Idle" ;

  case TH_ST_RELEASE:
    return "Release" ;

  case TH_ST_ACQUIRE:
    return "Acquire" ;

  case TH_ST_SELECT:
    return "Select" ;

  case TH_ST_DISPATCH:
    return "Dispatch" ;

  case TH_ST_SLOT_MOVE:
    return "Slot Move" ;

  case TH_ST_SLOT_FORCE_FREE:
    return "Slot Force Free" ;

  case TH_ST_SLOT_RESUME:
    return "Slot Resume" ;

  case TH_ST_SLOT_STEAL:
    return "Slot Steal" ;

  case TH_ST_IN_USE:
    return "In Use" ;

  default:
    return "Unknown" ;
  }
}

const char *LocoNetThrottleClass::getErrorStr( TH_ERROR Error )
{
  switch( Error )
  {
  case TH_ER_OK:

    return "Ok" ;

  case TH_ER_SLOT_IN_USE:

    return "In Use" ;

  case TH_ER_BUSY:

    return "Busy" ;

  case TH_ER_NOT_SELECTED:

    return "Not Sel" ;

  case TH_ER_NO_LOCO:

    return "No Loco" ;

  case TH_ER_NO_SLOTS:

    return "No Free Slots" ;

  default:

    return "Unknown" ;
  }
}

TH_SPEED_STEPS LocoNetThrottleClass::getSpeedSteps(void)
{
  return mySpeedSteps;
}

TH_ERROR LocoNetThrottleClass::setSpeedSteps(TH_SPEED_STEPS newSpeedSteps)
{
  mySpeedSteps = newSpeedSteps;
  if((myState == TH_ST_IN_USE) && ((myStatus1 & 0x07) != mySpeedSteps))
  {
        myStatus1 = (myStatus1 & 0xf8) | mySpeedSteps;
        LocoNet.send(OPC_SLOT_STAT1, mySlot, myStatus1);
  }
  updateSpeedSteps(mySpeedSteps, 1);
}

const char *LocoNetThrottleClass::getSpeedStepStr( TH_SPEED_STEPS speedStep )
{
  switch( speedStep )
  {
  case TH_SP_ST_28:       // 000=28 step/ 3 BYTE PKT regular mode
    return "28";
     
  case TH_SP_ST_28_TRI:  // 001=28 step. Generate Trinary packets for this Mobile ADR
    return "28 Tri";
  
  case TH_SP_ST_14:      // 010=14 step MODE
    return "14";
  
  case TH_SP_ST_128:     // 011=send 128 speed mode packets
    return "128";

  case TH_SP_ST_28_ADV:  // 100=28 Step decoder ,Allow Advanced DCC consisting
    return "28 Adv";
    
  case TH_SP_ST_128_ADV: // 111=128 Step decoder, Allow Advanced DCC consisting
    return "128 Adv";
  }

  return "Unknown";
}
