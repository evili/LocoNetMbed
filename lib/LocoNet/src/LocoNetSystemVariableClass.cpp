#include "LocoNetSystemVariableClass.h"

void LocoNetSystemVariableClass::init(uint8_t newMfgId, uint8_t newDevId, uint16_t newProductId, uint8_t newSwVersion)
{
  DeferredProcessingRequired = 0;
  DeferredSrcAddr = 0;
    
  mfgId = newMfgId ;
  devId = newDevId ;
        productId = newProductId ;
  swVersion = newSwVersion ;
}

uint8_t LocoNetSystemVariableClass::readSVStorage(uint16_t Offset )
{
        uint8_t retValue;
        
  if( Offset == SV_ADDR_EEPROM_SIZE)
#if (E2END==0x0FF)      /* E2END is defined in processor include */
                                                                return SV_EE_SZ_256;
#elif (E2END==0x1FF)
                                                                return SV_EE_SZ_512;
#elif (E2END==0x3FF)
                                                                return SV_EE_SZ_1024;
#elif (E2END==0x7FF)
                                                                return SV_EE_SZ_2048;
#elif (E2END==0xFFF)
                                                                return SV_EE_SZ_4096;
#else
                                                                return 0xFF;
#endif
  if( Offset == SV_ADDR_SW_VERSION )
    retValue = swVersion ;
    
  else
  {
    Offset -= 2;    // Map SV Address to EEPROM Offset - Skip SV_ADDR_EEPROM_SIZE & SV_ADDR_SW_VERSION
    retValue = eeprom_read_byte((uint8_t*)Offset);
  }
        return retValue;
}

uint8_t LocoNetSystemVariableClass::writeSVStorage(uint16_t Offset, uint8_t Value)
{
  Offset -= 2;      // Map SV Address to EEPROM Offset - Skip SV_ADDR_EEPROM_SIZE & SV_ADDR_SW_VERSION
  if( eeprom_read_byte((uint8_t*)Offset) != Value )
  {
    eeprom_write_byte((uint8_t*)Offset, Value);
    
    if(notifySVChanged)
      notifySVChanged(Offset+2);
  }    
  return eeprom_read_byte((uint8_t*)Offset) ;
}

uint8_t LocoNetSystemVariableClass::isSVStorageValid(uint16_t Offset)
{
  return (Offset >= SV_ADDR_EEPROM_SIZE ) && (Offset <= E2END + 2) ; 
}

bool LocoNetSystemVariableClass::CheckAddressRange(uint16_t startAddress, uint8_t Count)
{
#ifdef DEBUG_SV
  Serial.print("LNSV CheckAddressRange: Start: ");
  Serial.print(startAddress);
  Serial.print(" Size: ");
  Serial.println(Count);
#endif    
  while (Count != 0)
  {
    if (!isSVStorageValid(startAddress))
    {
       LocoNet.sendLongAck(42); // report invalid SV address error
       return 0;
    }
    startAddress++;
    Count--;
  }
  
  return 1; // all valid
}

uint16_t LocoNetSystemVariableClass::writeSVNodeId(uint16_t newNodeId)
{
#ifdef DEBUG_SV
    Serial.print("LNSV Write Node Addr: ");
    Serial.println(newNodeId);
#endif
    
    writeSVStorage(SV_ADDR_NODE_ID_H, newNodeId >> (byte) 8);
    writeSVStorage(SV_ADDR_NODE_ID_L, newNodeId & (byte) 0x00FF);
    
    return readSVNodeId();
}

uint16_t LocoNetSystemVariableClass::readSVNodeId(void)
{
    return (readSVStorage(SV_ADDR_NODE_ID_H) << 8 ) | readSVStorage(SV_ADDR_NODE_ID_L);
}

typedef union
{
word                   w;
struct { byte lo,hi; } b;
} U16_t;

typedef union
{
struct
{
  U16_t unDestinationId;
  U16_t unMfgIdDevIdOrSvAddress;
  U16_t unproductId;
  U16_t unSerialNumber;
}    stDecoded;
byte abPlain[8];
} SV_Addr_t;

SV_STATUS LocoNetSystemVariableClass::processMessage(lnMsg *LnPacket )
{
  SV_Addr_t unData ;
   
  if( ( LnPacket->sv.mesg_size != (byte) 0x10 ) ||
      ( LnPacket->sv.command != (byte) OPC_PEER_XFER ) ||
      ( LnPacket->sv.sv_type != (byte) 0x02 ) ||
      ( LnPacket->sv.sv_cmd & (byte) 0x40 ) ||
      ( ( LnPacket->sv.svx1 & (byte) 0xF0 ) != (byte) 0x10 ) ||
      ( ( LnPacket->sv.svx2 & (byte) 0xF0 ) != (byte) 0x10 ) )
    return SV_OK ;
 
  decodePeerData( &LnPacket->px, unData.abPlain ) ;

#ifdef DEBUG_SV
    Serial.print("LNSV Src: ");
    Serial.print(LnPacket->sv.src);
    Serial.print("  Dest: ");
    Serial.print(unData.stDecoded.unDestinationId.w);
    Serial.print("  CMD: ");
    Serial.println(LnPacket->sv.sv_cmd, HEX);
#endif
  if ((LnPacket->sv.sv_cmd != SV_DISCOVER) && 
      (LnPacket->sv.sv_cmd != SV_CHANGE_ADDRESS) && 
      (unData.stDecoded.unDestinationId.w != readSVNodeId()))
  {
#ifdef DEBUG_SV
    Serial.print("LNSV Dest Not Equal: ");
    Serial.println(readSVNodeId());
#endif
    return SV_OK;
  }

  switch( LnPacket->sv.sv_cmd )
  {
    case SV_WRITE_SINGLE:
        if (!CheckAddressRange(unData.stDecoded.unMfgIdDevIdOrSvAddress.w, 1)) return SV_ERROR;
        writeSVStorage(unData.stDecoded.unMfgIdDevIdOrSvAddress.w, unData.abPlain[4]);
        // fall through intended!
    case SV_READ_SINGLE:
        if (!CheckAddressRange(unData.stDecoded.unMfgIdDevIdOrSvAddress.w, 1)) return SV_ERROR;
        unData.abPlain[4] = readSVStorage(unData.stDecoded.unMfgIdDevIdOrSvAddress.w);
        break;

    case SV_WRITE_MASKED:
        if (!CheckAddressRange(unData.stDecoded.unMfgIdDevIdOrSvAddress.w, 1)) return SV_ERROR;
        // new scope for temporary local variables only
        {
         unsigned char ucOld = readSVStorage(unData.stDecoded.unMfgIdDevIdOrSvAddress.w) & (~unData.abPlain[5]);
         unsigned char ucNew = unData.abPlain[4] & unData.abPlain[5];
         writeSVStorage(unData.stDecoded.unMfgIdDevIdOrSvAddress.w, ucOld | ucNew);
        }
        unData.abPlain[4] = readSVStorage(unData.stDecoded.unMfgIdDevIdOrSvAddress.w);
        break;

    case SV_WRITE_QUAD:
        if (!CheckAddressRange(unData.stDecoded.unMfgIdDevIdOrSvAddress.w, 4)) return SV_ERROR;
        writeSVStorage(unData.stDecoded.unMfgIdDevIdOrSvAddress.w+0, unData.abPlain[4]);
        writeSVStorage(unData.stDecoded.unMfgIdDevIdOrSvAddress.w+1, unData.abPlain[5]);
        writeSVStorage(unData.stDecoded.unMfgIdDevIdOrSvAddress.w+2, unData.abPlain[6]);
        writeSVStorage(unData.stDecoded.unMfgIdDevIdOrSvAddress.w+3, unData.abPlain[7]);
        // fall through intended!
    case SV_READ_QUAD:
        if (!CheckAddressRange(unData.stDecoded.unMfgIdDevIdOrSvAddress.w, 4)) return SV_ERROR;
        unData.abPlain[4] = readSVStorage(unData.stDecoded.unMfgIdDevIdOrSvAddress.w+0);
        unData.abPlain[5] = readSVStorage(unData.stDecoded.unMfgIdDevIdOrSvAddress.w+1);
        unData.abPlain[6] = readSVStorage(unData.stDecoded.unMfgIdDevIdOrSvAddress.w+2);
        unData.abPlain[7] = readSVStorage(unData.stDecoded.unMfgIdDevIdOrSvAddress.w+3);
        break;

    case SV_DISCOVER:
        DeferredSrcAddr = LnPacket->sv.src ;
        DeferredProcessingRequired = 1 ;
        return SV_DEFERRED_PROCESSING_NEEDED ;
        break;
    
    case SV_IDENTIFY:
        unData.stDecoded.unDestinationId.w            = readSVNodeId();
        unData.stDecoded.unMfgIdDevIdOrSvAddress.b.hi = devId;
        unData.stDecoded.unMfgIdDevIdOrSvAddress.b.lo = mfgId ;
        unData.stDecoded.unproductId.w                = productId;
        unData.stDecoded.unSerialNumber.b.lo          = readSVStorage(SV_ADDR_SERIAL_NUMBER_L);
        unData.stDecoded.unSerialNumber.b.hi          = readSVStorage(SV_ADDR_SERIAL_NUMBER_H);
        break;

    case SV_CHANGE_ADDRESS:
        if((mfgId != unData.stDecoded.unMfgIdDevIdOrSvAddress.b.lo) || (devId != unData.stDecoded.unMfgIdDevIdOrSvAddress.b.hi))
          return SV_OK; // not addressed
        if(productId != unData.stDecoded.unproductId.w)
          return SV_OK; // not addressed
        if(readSVStorage(SV_ADDR_SERIAL_NUMBER_L) != unData.stDecoded.unSerialNumber.b.lo)
          return SV_OK; // not addressed
        if(readSVStorage(SV_ADDR_SERIAL_NUMBER_H) != unData.stDecoded.unSerialNumber.b.hi)
          return SV_OK; // not addressed
          
        if (writeSVNodeId(unData.stDecoded.unDestinationId.w) != unData.stDecoded.unDestinationId.w)
        {
          LocoNet.sendLongAck(44);  // failed to change address in non-volatile memory (not implemented or failed to write)
          return SV_OK ; // the LN reception was ok, we processed the message
        }
        break;

    case SV_RECONFIGURE:
        break;  // actual handling is done after sending out the reply

    default:
        LocoNet.sendLongAck(43); // not yet implemented
        return SV_ERROR;
  }
    
  encodePeerData( &LnPacket->px, unData.abPlain ); // recycling the received packet
    
  LnPacket->sv.sv_cmd |= 0x40;    // flag the message as reply
  
  LN_STATUS lnStatus = LocoNet.send(LnPacket, LN_BACKOFF_INITIAL);
        
#ifdef DEBUG_SV
  Serial.print("LNSV Send Response - Status: ");
  Serial.println(lnStatus);   // report status value from send attempt
#endif

  if (lnStatus != LN_DONE) {
    // failed to send the SV reply message.  Send will NOT be re-tried.
    LocoNet.sendLongAck(44);  // indicate failure to send the reply
  }
    
  if (LnPacket->sv.sv_cmd == (SV_RECONFIGURE | 0x40))
  {
    wdt_enable(WDTO_15MS);  // prepare for reset
    while (1) {}            // stop and wait for watchdog to knock us out
  }
   
  return SV_OK;
}

SV_STATUS LocoNetSystemVariableClass::doDeferredProcessing( void )
{
  if( DeferredProcessingRequired )
  {
    lnMsg msg ;
    SV_Addr_t unData ;
    
    msg.sv.command = (byte) OPC_PEER_XFER ;
    msg.sv.mesg_size = (byte) 0x10 ;
    msg.sv.src = DeferredSrcAddr ;
    msg.sv.sv_cmd = SV_DISCOVER | (byte) 0x40 ;
    msg.sv.sv_type = (byte) 0x02 ; 
    msg.sv.svx1 = (byte) 0x10 ;
    msg.sv.svx2 = (byte) 0x10 ;
    
    unData.stDecoded.unDestinationId.w            = readSVNodeId();
    unData.stDecoded.unMfgIdDevIdOrSvAddress.b.lo = mfgId;
    unData.stDecoded.unMfgIdDevIdOrSvAddress.b.hi = devId;
    unData.stDecoded.unproductId.w                = productId;
    unData.stDecoded.unSerialNumber.b.lo          = readSVStorage(SV_ADDR_SERIAL_NUMBER_L);
    unData.stDecoded.unSerialNumber.b.hi          = readSVStorage(SV_ADDR_SERIAL_NUMBER_H);
    
    encodePeerData( &msg.px, unData.abPlain );

    /* Note that this operation intentionally uses a "make one attempt to
       send to LocoNet" method here */
    if( sendLocoNetPacketTry( &msg, LN_BACKOFF_INITIAL + ( unData.stDecoded.unSerialNumber.b.lo % (byte) 10 ) ) != LN_DONE )
      return SV_DEFERRED_PROCESSING_NEEDED ;

    DeferredProcessingRequired = 0 ;
  }

  return SV_OK ;
}
