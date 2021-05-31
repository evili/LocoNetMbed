#ifndef __LOCONET_THROTTLE_H__
#define __LOCONET_THROTTLE_H__

#include "LocoNet.h"

typedef enum
{
  TH_ST_FREE   = 0,
  TH_ST_IDLE,
  TH_ST_RELEASE,
  TH_ST_ACQUIRE,
  TH_ST_SELECT,
  TH_ST_DISPATCH,
  TH_ST_SLOT_MOVE,
  TH_ST_SLOT_FORCE_FREE,
  TH_ST_SLOT_RESUME,
  TH_ST_SLOT_STEAL,
  TH_ST_IN_USE
} TH_STATE ;

typedef enum
{
  TH_ER_OK = 0,
  TH_ER_SLOT_IN_USE,
  TH_ER_BUSY,
  TH_ER_NOT_SELECTED,
  TH_ER_NO_LOCO,
  TH_ER_NO_SLOTS
} TH_ERROR ;


#define TH_OP_DEFERRED_SPEED 0x01
typedef enum
{
  TH_SP_ST_28      = 0,  // 000=28 step/ 3 BYTE PKT regular mode
  TH_SP_ST_28_TRI  = 1,  // 001=28 step. Generate Trinary packets for this Mobile ADR
  TH_SP_ST_14      = 2,  // 010=14 step MODE
  TH_SP_ST_128     = 3,  // 011=send 128 speed mode packets
  TH_SP_ST_28_ADV  = 4,  // 100=28 Step decoder ,Allow Advanced DCC consisting
  TH_SP_ST_128_ADV = 7   // 111=128 Step decoder, Allow Advanced DCC consisting
} TH_SPEED_STEPS;



class LocoNetThrottleClass
{
  private:
        TH_STATE           myState ;         // State of throttle
        uint16_t           myTicksSinceLastAction ;
        uint16_t           myThrottleId ;               // Id of throttle
        uint8_t            mySlot ;          // Master Slot index
        uint16_t           myAddress ;       // Decoder Address
        uint8_t            mySpeed ;         // Loco Speed
        uint8_t            myDeferredSpeed ; // Deferred Loco Speed setting
        uint8_t            myStatus1 ;       // Stat1
        uint8_t            myDirFunc0to4 ;   // Direction
        uint8_t            myFunc5to8 ;       // Direction
        uint8_t            myUserData ;
        uint8_t            myOptions ;
        uint32_t           myLastTimerMillis;
        TH_SPEED_STEPS mySpeedSteps;
        
        void updateAddress(uint16_t Address, uint8_t ForceNotify );
        void updateSpeed(uint8_t Speed, uint8_t ForceNotify );
        void updateState(TH_STATE State, uint8_t ForceNotify );
        void updateStatus1(uint8_t Status, uint8_t ForceNotify );
        void updateDirectionAndFunctions(uint8_t DirFunc0to4, uint8_t ForceNotify );
        void updateFunctions5to8(uint8_t Func5to8, uint8_t ForceNotify );
        void updateSpeedSteps(TH_SPEED_STEPS SpeedSteps, uint8_t ForceNotify);
  
  public:
        void init(uint8_t UserData, uint8_t Options, uint16_t ThrottleId ) ;

        void processMessage(lnMsg *LnPacket ) ;
        void process100msActions(void);

        uint16_t getAddress(void) ;
        TH_ERROR setAddress(uint16_t Address) ;
        TH_ERROR stealAddress(uint16_t Address) ;
        TH_ERROR resumeAddress(uint16_t Address, uint8_t LastSlot) ;
        TH_ERROR dispatchAddress(void) ;
        TH_ERROR acquireAddress(void) ;
        TH_ERROR releaseAddress(void) ;
    TH_ERROR idleAddress(void) ;
    TH_ERROR freeAddress(void) ;
    
        TH_ERROR dispatchAddress(uint16_t Address) ;
        TH_ERROR freeAddressForce(uint16_t Address) ;

        uint8_t getSpeed(void) ;
        TH_ERROR setSpeed(uint8_t Speed) ;

        uint8_t getDirection(void) ;
        TH_ERROR setDirection(uint8_t Direction) ;

        uint8_t getFunction(uint8_t Function) ;
        TH_ERROR setFunction(uint8_t Function, uint8_t Value) ;
        TH_ERROR setDirFunc0to4Direct(uint8_t Value) ;
        TH_ERROR setFunc5to8Direct(uint8_t Value) ;
        
        TH_SPEED_STEPS getSpeedSteps(void);
        TH_ERROR setSpeedSteps(TH_SPEED_STEPS newSpeedSteps);

        TH_STATE getState(void) ;
        uint8_t getSlot() { return mySlot ; }

        const char *getStateStr( TH_STATE State );
        const char *getErrorStr( TH_ERROR Error );
        const char *getSpeedStepStr( TH_SPEED_STEPS speedStep );
};

#if defined (__cplusplus)
        extern "C" {
#endif

// Throttle notify Call-back functions
extern void notifyThrottleAddress( uint8_t UserData, TH_STATE State, uint16_t Address, uint8_t Slot ) __attribute__ ((weak));
extern void notifyThrottleSpeed( uint8_t UserData, TH_STATE State, uint8_t Speed ) __attribute__ ((weak));
extern void notifyThrottleDirection( uint8_t UserData, TH_STATE State, uint8_t Direction ) __attribute__ ((weak));
extern void notifyThrottleFunction( uint8_t UserData, uint8_t Function, uint8_t Value ) __attribute__ ((weak));
extern void notifyThrottleSlotStatus( uint8_t UserData, uint8_t Status ) __attribute__ ((weak));
extern void notifyThrottleSpeedSteps( uint8_t UserData, TH_SPEED_STEPS SpeedSteps ) __attribute__ ((weak));
extern void notifyThrottleError( uint8_t UserData, TH_ERROR Error ) __attribute__ ((weak));
extern void notifyThrottleState( uint8_t UserData, TH_STATE PrevState, TH_STATE State ) __attribute__ ((weak));

#if defined (__cplusplus)
}
#endif

#endif // __LOCONET_THROTTLE_H__
