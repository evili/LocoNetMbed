#ifndef __LOCONET_CLASS_H__
#define __LOCONET_CLASS_H__

#include "LocoNet.h"

typedef enum
{
        LN_CD_BACKOFF = 0,
        LN_PRIO_BACKOFF,
        LN_NETWORK_BUSY,
        LN_DONE,
        LN_COLLISION,
        LN_UNKNOWN_ERROR,
        LN_RETRY_ERROR
} LN_STATUS ;

class LocoNetClass
{
private:
  LnBuf LnBuffer ;
  uint8_t _tx_pin;
  void setTxPin(uint8_t txPin);
  
public:
  LocoNetClass();
    void        init(void);
    void        init(uint8_t txPin);
    bool     available(void);
    uint8_t     length(void);
    lnMsg*      receive(void);
    LN_STATUS   send(lnMsg *TxPacket);
    LN_STATUS   send(lnMsg *TxPacket, uint8_t PrioDelay);
    LN_STATUS   send(uint8_t OpCode, uint8_t Data1, uint8_t Data2);
    LN_STATUS   send(uint8_t OpCode, uint8_t Data1, uint8_t Data2, uint8_t PrioDelay);
    LN_STATUS   sendLongAck(uint8_t ucCode);
    
    LnBufStats* getStats(void);
    
        const char*     getStatusStr(LN_STATUS Status);
    
    uint8_t processSwitchSensorMessage( lnMsg *LnPacket ) ;
    uint8_t processPowerTransponderMessage( lnMsg *LnPacket ) ;
        
    LN_STATUS requestSwitch( uint16_t Address, uint8_t Output, uint8_t Direction ) ;
    LN_STATUS reportSwitch( uint16_t Address ) ;
    LN_STATUS reportSensor( uint16_t Address, uint8_t State ) ;
    LN_STATUS reportPower( uint8_t State ) ;
};

#endif // __LOCONET_CLASS_H__
