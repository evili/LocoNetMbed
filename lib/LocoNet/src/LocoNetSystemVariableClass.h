#ifndef __LOCONET_SYSTEM_VARIABLE_H__
#define __LOCONET_SYSTEM_VARIABLE_H__

#include "LocoNet.h"

#ifndef E2END
#define E2END (1024u)
#endif

/************************************************************************************
    SV (System Variable Handling
************************************************************************************/

typedef enum
{
  SV_EE_SZ_256 = 0,
  SV_EE_SZ_512 = 1,
  SV_EE_SZ_1024 = 2,
  SV_EE_SZ_2048 = 3,
  SV_EE_SZ_4096 = 4,
  SV_EE_SZ_8192 = 5
} SV_EE_SIZE ;

typedef enum
{
  SV_WRITE_SINGLE = 0x01,
  SV_READ_SINGLE = 0x02,
  SV_WRITE_MASKED = 0x03,
  SV_WRITE_QUAD = 0x05,
  SV_READ_QUAD = 0x06,
  SV_DISCOVER = 0x07,
  SV_IDENTIFY = 0x08,
  SV_CHANGE_ADDRESS = 0x09,
  SV_RECONFIGURE = 0x0F
} SV_CMD ;

typedef enum
{
  SV_ADDR_EEPROM_SIZE = 1,
  SV_ADDR_SW_VERSION = 2,
  SV_ADDR_NODE_ID_L = 3,
  SV_ADDR_NODE_ID_H = 4,
  SV_ADDR_SERIAL_NUMBER_L = 5,
  SV_ADDR_SERIAL_NUMBER_H = 6,
  SV_ADDR_USER_BASE = 7,
} SV_ADDR ;

typedef enum
{
  SV_OK = 0,
  SV_ERROR = 1,
  SV_DEFERRED_PROCESSING_NEEDED = 2
} SV_STATUS ;

class LocoNetSystemVariableClass
{
  private:
        uint8_t         mfgId ;
        uint8_t         devId ;
        uint16_t        productId ;
  uint8_t   swVersion ;
    
  uint8_t DeferredProcessingRequired ;
  uint8_t DeferredSrcAddr ;
    
  public:
        /** Checks whether the given Offset is a valid value.
         *
         * Returns:
         *              True - if the given Offset is valid. False Otherwise.
         */
    uint8_t isSVStorageValid(uint16_t Offset);
        
        /** Read the NodeId (Address) for SV programming of this module.
         *
         * This method accesses multiple special EEPROM locations.
         */
    uint16_t readSVNodeId(void);
        
        /** Write the NodeId (Address) for SV programming of this module.
         *
         * This method accesses multiple special EEPROM locations.
         */
    uint16_t writeSVNodeId(uint16_t newNodeId);
        
        /**
         * Checks whether all addresses of an address range are valid (defers to
         * isSVStorageValid()). Sends a notification for the first invalid address
         * (long Ack with a value of 42).
         *
         *      TODO: There is a Type error in this method. Return type is bool, but
         *              actual returned values are Integer.
         *
         * Returns:
         *              0 if at least one address of the range is not valid.
         *              1 if all addresses out of the range are valid.
         */
    bool CheckAddressRange(uint16_t startAddress, uint8_t Count);
        void init(uint8_t newMfgId, uint8_t newDevId, uint16_t newProductId, uint8_t newSwVersion);
        
        /**
         * Check whether a message is an SV programming message. If so, the message
         * is processed.
         * Call this message in your main loop to implement SV programming.
         *
         * TODO: This method should be updated to reflect whether the message has
         *      been consumed.
         *
         * Note that this method will not send out replies.
         *
         * Returns:
         *              SV_OK - the message was or was not an SV programming message.
                                It may or may not have been consumed.
         *              SV_DEFERRED_PROCESSING_NEEDED - the message was an SV programming
                                message and has been consumed. doDeferredProcessing() must be
                                called to actually process the message.
         *              SV_ERROR - the message was an SV programming message and carried
                                an unsupported OPCODE.
         *
         */
        SV_STATUS processMessage(lnMsg *LnPacket );
        
    /** Read a value from the given EEPROM offset.
     *
     * There are two special values for the Offset parameter:
     *  SV_ADDR_EEPROM_SIZE - Return the size of the EEPROM
     *  SV_ADDR_SW_VERSION - Return the value of swVersion
     *  3 and on - Return the byte stored in the EEPROM at location (Offset - 2)
     *
     * Parameters:
     *          Offset: The offset into the EEPROM. Despite the value being passed as 2 Bytes, only the lower byte is respected.
     *
     * Returns:
     *          A Byte containing the EEPROM size, the software version or contents of the EEPROM.
     *
     */
    uint8_t readSVStorage(uint16_t Offset );
    
    /** Write the given value to the given Offset in EEPROM.
     *
     * TODO: Writes to Offset 0 and 1 will cause data corruption.
     *
     * Fires notifySVChanged(Offset), if the value actually chaned.
     *
     * Returns:
     *          A Byte containing the new EEPROM value (even if unchanged).
     */
    uint8_t writeSVStorage(uint16_t Offset, uint8_t Value);
    
        /**
         * Attempts to send a reply to an SV programming message.
         * This method will repeatedly try to send the message, until it succeeds.
         *
         * Returns:
         *              SV_OK - Reply was successfully sent.
         *              SV_DEFERRED_PROCESSING_NEEDED - Reply was not sent, a later retry is needed.
         */
    SV_STATUS doDeferredProcessing( void );
};



#if defined (__cplusplus)
        extern "C" {
#endif

// System Variable notify Call-back functions
extern void notifySVChanged(uint16_t Offset) __attribute__ ((weak));

#if defined (__cplusplus)
}
#endif


#endif // __LOCONET_SYSTEM_VARIABLE_H__
