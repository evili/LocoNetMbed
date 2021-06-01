#include "LocoNetClass.h"
#include "ln_sw_uart.h"

const char * LoconetStatusStrings[] = {
        "CD Backoff",
        "Prio Backoff",
        "Network Busy",
        "Done",
        "Collision",
        "Unknown Error",
        "Retry Error"
};

LocoNetClass::LocoNetClass()
{
}

void LocoNetClass::init(void)
{
  init(LOCONET_TX_DEFAULT_PIN);
}


void LocoNetClass::init(PinName txPin)
{
  init(txPin, LOCONET_RX_DEFAULT_PIN);
}

void LocoNetClass::init(PinName txPin, PinName rxPin)
{
  initLnBuf(&LnBuffer) ;
  setTxPin(txPin);
  setRxPin(rxPin);
  initLocoNetHardware(&LnBuffer, );
}

const char* LocoNetClass::getStatusStr(LN_STATUS Status)
{
  if((Status >= LN_CD_BACKOFF) && (Status <= LN_RETRY_ERROR))
    return LoconetStatusStrings[Status];
        
  return "Invalid Status";
}


void LocoNetClass::setTxPin(PinName txPin) {
  _txPin = new DigitalOut(txPin);
}

void LocoNetClass::setRxPin(PinName rxPin) {
  _rxPin = new Interruptin(rxPin);
}


// Check to see if any messages is ready to receive()?
bool LocoNetClass::available(void)
{
  return lnPacketReady(&LnBuffer);
}

// Check the size in bytes of waiting message
uint8_t LocoNetClass::length(void)
{
  if (lnPacketReady(&LnBuffer))
  {
                lnMsg* m = (lnMsg *)&(LnBuffer.Buf[ LnBuffer.ReadIndex ]);
                return getLnMsgSize(m);
  } 
  else
                return 0;
}

lnMsg *LocoNetClass::receive() {
    return recvLnMsg(&LnBuffer);
}

/*
  Send a LocoNet message, using the default priority delay.

  When an attempt to send fails, this method will continue to try to re-send
  the message until it is successfully sent or until the maximum retry
  limit is reached.

  Return value is one of:
    LN_DONE -         Indicates successful send of the message
    LN_RETRY_ERROR -  Could not successfully send the message within 
                        LN_TX_RETRIES_MAX attempts
    LN_UNKNOWN -      Indicates an abnormal exit condition for the send attempt.
                        In this case, it is recommended to make another send 
                        attempt.
*/
LN_STATUS LocoNetClass::send(lnMsg *pPacket)
{
  return send(pPacket, LN_BACKOFF_INITIAL);
}

/*
  Send a LocoNet message, using an argument for the priority delay.

  When an attempt to send fails, this method will continue to try to re-send
  the message until it is successfully sent or until the maximum retry
  limit is reached.

  Return value is one of:
    LN_DONE -         Indicates successful send of the message
    LN_RETRY_ERROR -  Could not successfully send the message within 
                        LN_TX_RETRIES_MAX attempts
    LN_UNKNOWN -      Indicates an abnormal exit condition for the send attempt.
                        In this case, it is recommended to make another send 
                        attempt.
*/
LN_STATUS LocoNetClass::send(lnMsg *pPacket, uint8_t ucPrioDelay)
{
  unsigned char ucTry;
  LN_STATUS enReturn;
  unsigned char ucWaitForEnterBackoff;

  for (ucTry = 0; ucTry < LN_TX_RETRIES_MAX; ucTry++)
  {

    // wait previous traffic and than prio delay and than try tx
    ucWaitForEnterBackoff = 1;  // don't want to abort do/while loop before
    do                          // we did not see the backoff state once
    {
      enReturn = sendLocoNetPacketTry(pPacket, ucPrioDelay);

      if (enReturn == LN_DONE)  // success?
        return LN_DONE;

      if (enReturn == LN_PRIO_BACKOFF)
        ucWaitForEnterBackoff = 0; // now entered backoff -> next state != LN_BACKOFF is worth incrementing the try counter
    }
    while ((enReturn == LN_CD_BACKOFF) ||                             // waiting CD backoff
    (enReturn == LN_PRIO_BACKOFF) ||                           // waiting master+prio backoff
    ((enReturn == LN_NETWORK_BUSY) && ucWaitForEnterBackoff)); // or within any traffic unfinished
    // failed -> next try going to higher prio = smaller prio delay
    if (ucPrioDelay > LN_BACKOFF_MIN)
      ucPrioDelay--;
  }
  LnBuffer.Stats.TxErrors++ ;
  return LN_RETRY_ERROR;
}
/*
  Create a four-byte LocoNet message from the three method parameters plus a 
  computed checksum, and send that message to LocoNet using a default priority 
  delay.

  When an attempt to send fails, this method will continue to try to re-send
  the message until it is successfully sent or until the maximum retry
  limit is reached.

  Return value is one of:
    LN_DONE -         Indicates successful send of the message
    LN_RETRY_ERROR -  Could not successfully send the message within 
                        LN_TX_RETRIES_MAX attempts
    LN_UNKNOWN -      Indicates an abnormal exit condition for the send attempt.
                        In this case, it is recommended to make another send 
                        attempt.
*/

LN_STATUS LocoNetClass::send( uint8_t OpCode, uint8_t Data1, uint8_t Data2 )
{
  lnMsg SendPacket ;

  SendPacket.data[ 0 ] = OpCode ;
  SendPacket.data[ 1 ] = Data1 ;
  SendPacket.data[ 2 ] = Data2 ;

  return send( &SendPacket ) ;
}

/*
  Create a four-byte LocoNet message from the three method parameters plus a 
  computed checksum, and send that message to LocoNet using a parameter for the
  priority delay.

  This method will make exactly one attempt to send the LocoNet message which
  may or may not succeed.  Code which uses this method must check the return
  status value to determine if the send should be re-tried.  Such code must also
  implement a retry limit counter.

  Return value is one of:
    LN_DONE -         Indicates successful send of the message.
    LN_CD_BACKOFF -   Indicates that the message cannot be sent at this time
                        because the LocoNet state machine is in the "Carrier Detect
                        Backoff" phase.  This should not count against the "retry"
                        count.
    LN_PRIO_BACKOFF - Indicates that the message cannot be sent at this time
                        because the LocoNet state machine is in the "Priority 
                        Backoff" phase.  This should not count against the "retry" 
                        count.
    LN_NETWORK_BUSY - Indicates that the message cannot be sent at this time
                        because some other LocoNet agent is currently sending
                        a message.  This should not count against the "retry"
                        count.
    LN_COLLISSION -   Indicates that an attempt was made to send the message 
                        but that the message was corrupted by some other LocoNet
                        agent's LocoNe traffic.  The retry counter should be 
                        decremented and another send should be attempted if the 
                        retry limit has not been reached.
    LN_UNKNOWN -      Indicates an abnormal exit condition for the send attempt.
                        In this case, it is recommended to decrement the retry 
                        counter and make another send attempt if the retry limit
                        has not been reached.
*/
LN_STATUS LocoNetClass::send( uint8_t OpCode, uint8_t Data1, uint8_t Data2, uint8_t PrioDelay )
{
  lnMsg SendPacket ;

  SendPacket.data[ 0 ] = OpCode ;
  SendPacket.data[ 1 ] = Data1 ;
  SendPacket.data[ 2 ] = Data2 ;

  return sendLocoNetPacketTry( &SendPacket, PrioDelay ) ;
}

/* send a LONG_ACK (LACK) message to Loconet as a response to an OPC_PEER_XFER 
   message, using the method parameter as the error code in the LONG_ACK message.

  When an attempt to send fails, this method will continue to try to re-send
  the message until it is successfully sent or until the maximum retry
  limit is reached.

  Return value is one of:
    LN_DONE -         Indicates successful send of the message.
    LN_RETRY_ERROR -  Indicates that the method could not successfully send the
                        message within LN_TX_RETRIES_MAX attempts.
    LN_UNKNOWN -      Indicates an abnormal exit condition for the send attempt.
                        In this case, it is recommended to make another send 
                        attempt.
*/
LN_STATUS LocoNetClass::sendLongAck(uint8_t ucCode)
{
  lnMsg SendPacket ;

  SendPacket.data[ 0 ] = OPC_LONG_ACK ;
  SendPacket.data[ 1 ] = OPC_PEER_XFER-0x80 ;
  SendPacket.data[ 2 ] = ucCode ;

  return send( &SendPacket ) ;
}

LnBufStats* LocoNetClass::getStats(void)
{
    return &LnBuffer.Stats;
}

LN_STATUS LocoNetClass::reportPower(uint8_t State)
{
  lnMsg SendPacket ;

  if(State)
    SendPacket.data[ 0 ] = OPC_GPON ;
  else
    SendPacket.data[ 0 ] = OPC_GPOFF ;

  return send( &SendPacket ) ;
}

uint8_t LocoNetClass::processSwitchSensorMessage( lnMsg *LnPacket )
{
  uint16_t Address ;
  uint8_t  Direction ;
  uint8_t  Output ;
  uint8_t  ConsumedFlag = 1 ;

  Address = (LnPacket->srq.sw1 | ( ( LnPacket->srq.sw2 & 0x0F ) << 7 )) ;
  if( LnPacket->sr.command != OPC_INPUT_REP )
    Address++;

  switch( LnPacket->sr.command )
  {
  case OPC_INPUT_REP:
    Address <<= 1 ;
    Address += ( LnPacket->ir.in2 & OPC_INPUT_REP_SW ) ? 2 : 1 ;

    if(notifySensor)
      notifySensor( Address, LnPacket->ir.in2 & OPC_INPUT_REP_HI ) ;
    break ;

  case OPC_GPON:
    if(notifyPower)
      notifyPower( 1 );
    break ;

  case OPC_GPOFF:
    if(notifyPower)
      notifyPower( 0 );
    break ;

  case OPC_SW_REQ:
    if(notifySwitchRequest)
      notifySwitchRequest( Address, LnPacket->srq.sw2 & OPC_SW_REQ_OUT, LnPacket->srq.sw2 & OPC_SW_REQ_DIR ) ;
    break ;

  case OPC_SW_REP:
        if(LnPacket->srp.sn2 & OPC_SW_REP_INPUTS)
        {
        if(notifySwitchReport)
        notifySwitchReport( Address, LnPacket->srp.sn2 & OPC_SW_REP_HI, LnPacket->srp.sn2 & OPC_SW_REP_SW ) ;
    }
    else
    {
        if(notifySwitchOutputsReport)
        notifySwitchOutputsReport( Address, LnPacket->srp.sn2 & OPC_SW_REP_CLOSED, LnPacket->srp.sn2 & OPC_SW_REP_THROWN ) ;
    }
    break ;

  case OPC_SW_STATE:
    Direction = LnPacket->srq.sw2 & OPC_SW_REQ_DIR ;
    Output = LnPacket->srq.sw2 & OPC_SW_REQ_OUT ;

    if(notifySwitchState)
      notifySwitchState( Address, Output, Direction ) ;
    break;

  case OPC_SW_ACK:
    break ;
    
  case OPC_MULTI_SENSE:
  
    switch( LnPacket->data[1] & OPC_MULTI_SENSE_MSG )
    {
      case OPC_MULTI_SENSE_DEVICE_INFO:
        // This is a PM42 power event.

        /* Working on porting this */
        if(notifyMultiSensePower)
        {
          uint8_t pCMD ;
          pCMD = (LnPacket->msdi.arg3 & 0xF0) ;

          if ((pCMD == 0x30) || (pCMD == 0x10))
          {
            // Autoreverse & Circuitbreaker
            uint8_t cm1 = LnPacket->msdi.arg3 ;
            uint8_t cm2 = LnPacket->msdi.arg4 ;

            uint8_t Mode ; // 0 = AutoReversing 1 = CircuitBreaker

            uint8_t boardID = ((LnPacket->msdi.arg2 + 1) + ((LnPacket->msdi.arg1 & 0x1) == 1) ? 128 : 0) ;

            // Report 4 Sub-Districts for a PM4x
            uint8_t d = 1 ;
            for ( uint8_t i = 1; i < 5; i++ )
            {
              if ((cm1 & d) != 0)
              {
                Mode = 0 ;
              } else {
                Mode = 1 ;
              }
              Direction = cm2 & d ;
              d = d * 2 ;
              notifyMultiSensePower( boardID, i, Mode, Direction ) ; // BoardID, Subdistrict, Mode, Direction
            }
          } else if (pCMD == 0x70) {
              // Programming
          } else if (pCMD == 0x00) {
              // Device type report
          }
        }
        break ;

      case OPC_MULTI_SENSE_ABSENT:
      case OPC_MULTI_SENSE_PRESENT:
        // Transponding Event
        if(notifyMultiSenseTransponder)
        {
          uint16_t Locoaddr ;
          uint8_t  Present ;
          char     Zone ;

          Address = LnPacket->mstr.zone + ((LnPacket->mstr.type & 0x1F) << 7) ;
          Present = (LnPacket->mstr.type & 0x20) != 0 ? true : false ;
          
          Address++ ;
          
                if( LnPacket->mstr.adr1 == 0x7D)
                  Locoaddr = LnPacket->mstr.adr2 ;
                else
                  Locoaddr = (LnPacket->mstr.adr1 * 128) + LnPacket->mstr.adr2 ;
                  
                if ( (LnPacket->mstr.zone&0x0F) == 0x00 ) Zone = 'A' ;
          else if ( (LnPacket->mstr.zone&0x0F) == 0x02 ) Zone = 'B' ;
          else if ( (LnPacket->mstr.zone&0x0F) == 0x04 ) Zone = 'C' ;
          else if ( (LnPacket->mstr.zone&0x0F) == 0x06 ) Zone = 'D' ;
          else if ( (LnPacket->mstr.zone&0x0F) == 0x08 ) Zone = 'E' ;
          else if ( (LnPacket->mstr.zone&0x0F) == 0x0A ) Zone = 'F' ;
          else if ( (LnPacket->mstr.zone&0x0F) == 0x0C ) Zone = 'G' ;
          else if ( (LnPacket->mstr.zone&0x0F) == 0x0E ) Zone = 'H' ;
          else Zone = LnPacket->mstr.zone&0x0F ;
                
            notifyMultiSenseTransponder( Address, Zone, Locoaddr, Present ) ;
          break ;
        }
    }
    break ;

  case OPC_LONG_ACK:
    if( LnPacket->lack.opcode == (OPC_SW_STATE & 0x7F ) )
    {
      Direction = LnPacket->lack.ack1 & 0x01 ;
      if(notifyLongAck)
              notifyLongAck(LnPacket->data[1], LnPacket->data[2]);    
    }
    else
      ConsumedFlag = 0 ;
    break;

  default:
    ConsumedFlag = 0 ;
  }

  return ConsumedFlag ;
}

LN_STATUS LocoNetClass::requestSwitch( uint16_t Address, uint8_t Output, uint8_t Direction )
{
  uint8_t AddrH = (--Address >> 7) & 0x0F ;
  uint8_t AddrL = Address & 0x7F ;

  if( Output )
    AddrH |= OPC_SW_REQ_OUT ;

  if( Direction )
    AddrH |= OPC_SW_REQ_DIR ;

  return send( OPC_SW_REQ, AddrL, AddrH ) ;
}

LN_STATUS LocoNetClass::reportSwitch( uint16_t Address )
{
  Address -= 1;
  return send( OPC_SW_STATE, (Address & 0x7F), ((Address >> 7) & 0x0F) ) ;
}

LN_STATUS LocoNetClass::reportSensor( uint16_t Address, uint8_t State )
{
        uint8_t AddrH = ( (--Address >> 8) & 0x0F ) | OPC_INPUT_REP_CB ;
        uint8_t AddrL = ( Address >> 1 ) & 0x7F ;
        if( Address % 2)
                AddrH |= OPC_INPUT_REP_SW ;

        if( State )
                AddrH |= OPC_INPUT_REP_HI  ;

  return send( OPC_INPUT_REP, AddrL, AddrH ) ;
}

LocoNetClass LocoNet = LocoNetClass();

