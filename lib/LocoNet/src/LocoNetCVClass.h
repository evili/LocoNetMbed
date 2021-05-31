#ifndef __LOCONET_CV_H__
#define __LOCONET_CV_H__

#include "LocoNet.h"

//
// LNCV error codes
// Used by the LNCV callbacks to signal what kind of error has occurred.
//

// Error-codes for write-requests
#define LNCV_LACK_ERROR_GENERIC (0)
// Unsupported/non-existing CV
#define LNCV_LACK_ERROR_UNSUPPORTED (1)
// CV is read only
#define LNCV_LACK_ERROR_READONLY (2)
// Value out of range
#define LNCV_LACK_ERROR_OUTOFRANGE (3)
// Everything OK
#define LNCV_LACK_OK (127)

// the valid range for module addresses (CV0) as per the LNCV spec.
#define LNCV_MIN_MODULEADDR (0)
#define LNCV_MAX_MODULEADDR (65534)


class LocoNetCVClass
{
  private:
    void makeLNCVresponse( UhlenbrockMsg & ub, uint8_t originalSource, uint16_t first, uint16_t second, uint16_t third, uint8_t last );
    
      // Computes the PXCT byte from the data bytes in the given UhlenbrockMsg.
    void computePXCTFromBytes( UhlenbrockMsg & ub) ;

      // Computes the correct data bytes using the containes PXCT byte
    void computeBytesFromPXCT( UhlenbrockMsg & ub) ;

      // Computes an address from a low- and a high-byte
    uint16_t getAddress(uint8_t lower, uint8_t higher) ;

  public:
          //Call this method when you want to implement a module that can be configured via Uhlenbrock LNVC messages
        uint8_t processLNCVMessage( lnMsg *LnPacket ) ;
};

/**
 * TODO: General LNCV documentation
 * Pick an ArtNr
 * Implement your code to the following behaviour...
 */

#if defined (__cplusplus)
        extern "C" {
#endif

// LNCV notify Call-back functions
// Negative return codes will result in no message being sent.
// Where a value response is appropriate, a return value of LNCV_LACK_OK will trigger the
// response being sent.
// Other values greater than 0 will result in a LACK message being sent.
// When no value result is appropriate, LNCV_LACK_OK will be sent as a LACK.


/**
 * Notification that an LNCVDiscover message was sent. If a module wants to react to this,
 * It should return LNCV_LACK_OK and set ArtNr and ModuleAddress accordingly.
 * A response just as in the case of notifyLNCVProgrammingStart will be generated.
 * If a module responds to a LNCVDiscover, it should apparently enter programming mode immediately.
 */
extern int8_t notifyLNCVdiscover( uint16_t & ArtNr, uint16_t & ModuleAddress ) __attribute__ ((weak));;

/**
 * Notification that a LNCVProgrammingStart message was received. Application code should process this message and
 * set the return code to LNCV_LACK_OK in case this message was intended for this module (i.e., the addresses match).
 * In case ArtNr and/or ModuleAddress were Broadcast addresses, the Application Code should replace them by their
 * real values.
 * The calling code will then generate an appropriate ACK message.
 * A return code different than LACK_LNCV_OK will result in no response being sent.
 */
extern int8_t notifyLNCVprogrammingStart ( uint16_t & ArtNr, uint16_t & ModuleAddress ) __attribute__ ((weak));

/**
 * Notification that a LNCV read request message was received. Application code should process this message,
 * set the lncvValue to its respective value and set an appropriate return code.
 * return LNCV_LACK_OK leads the calling code to create a response containing lncvValue.
 * return code >= 0 leads to a NACK being sent.
 * return code < 0 will result in no reaction.
 */
extern int8_t notifyLNCVread ( uint16_t ArtNr, uint16_t lncvAddress, uint16_t, uint16_t & lncvValue ) __attribute__ ((weak));

/**
 * Notification that a LNCV value should be written. Application code should process this message and
 * set an appropriate return code.
 * Note 1: LNCV 0 is spec'd to be the ModuleAddress.
 * Note 2: Changes to LNCV 0 must be reflected IMMEDIATELY! E.g. the programmingStop command will
 * be sent using the new address.
 *
 * return codes >= 0 will result in a LACK containing the return code being sent.
 * return codes < 0 will result in no reaction.
 */
extern int8_t notifyLNCVwrite ( uint16_t ArtNr, uint16_t lncvAddress, uint16_t lncvValue ) __attribute__ ((weak));

/**
 * Notification that an LNCV Programming Stop message was received.
 * This message is noch ACKed, thus does not require a result to be returned from the application.
 */
extern void notifyLNCVprogrammingStop( uint16_t ArtNr, uint16_t ModuleAddress ) __attribute__ ((weak));

#if defined (__cplusplus)
}
#endif


#endif // __LOCONET_CV_H__
