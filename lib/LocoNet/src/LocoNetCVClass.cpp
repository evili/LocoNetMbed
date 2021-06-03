#include "LocoNetCVClass.h"

 /*****************************************************************************
 *      DESCRIPTION
 *      This module provides functions that manage the LNCV-specific programming protocol
 * 
 *****************************************************************************/

// Adresses for the 'SRC' part of an UhlenbrockMsg
#define LNCV_SRC_MASTER 0x00
#define LNCV_SRC_KPU 0x01
// KPU is, e.g., an IntelliBox
// 0x02 has no associated meaning
#define LNCV_SRC_TWINBOX_FRED 0x03
#define LNCV_SRC_IBSWITCH 0x04
#define LNCV_SRC_MODULE 0x05

// Adresses for the 'DSTL'/'DSTH' part of an UhlenbrockMsg
#define LNCV_BROADCAST_DSTL 0x00
#define LNCV_BROADCAST_DSTH 0x00
#define LNCV_INTELLIBOX_SPU_DSTL 'I'
#define LNCV_INTELLIBOX_SPU_DSTH 'B'
#define LNCV_INTELLIBOX_KPU_DSTL 'I'
#define LNCV_INTELLIBOX_KPU_DSTH 'K'
#define LNCV_TWINBOX_DSTH 'T'
// For TwinBox, DSTL can be anything from 0 to 15
#define LNCV_IBSWITCH_KPU_DSTL 'I'
#define LNCV_IBSWITCH_KPU_DSTH 'S'
#define LNCV_MODULE_DSTL 0x05
#define LNCV_MODULE_DSTH 0x00

// Request IDs
#define LNCV_REQID_CFGREAD 31
#define LNCV_REQID_CFGWRITE 32
#define LNCV_REQID_CFGREQUEST 33

// Flags for the 7th data Byte
#define LNCV_FLAG_PRON 0x80
#define LNCV_FLAG_PROFF 0x40
#define LNCV_FLAG_RO 0x01
// other flags are currently unused

//#define DEBUG_OUTPUT
#undef DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
//#define DEBUG(x) Serial.print(F(x))
#define DEBUG(x) printf("%s", x)
#else
#define DEBUG(x)
#endif

#ifdef DEBUG_OUTPUT
void printPacket(lnMsg* LnPacket) {
  printf("LoconetPacket %x %x %x %x %x %x %x",
	 LnPacket->ub.command,
	 LnPacket->ub.mesg_size,
	 LnPacket->ub.SRC,
	 LnPacket->ub.DSTL,
	 LnPacket->ub.DSTH,
	 LnPacket->ub.ReqId,
	 LnPacket->ub.PXCT1);
  for (int i(0); i < 7; ++i) {
    printf(" %x" LnPacket->ub.payload.D[i]);
  }
  printf("\n");
}
#endif

uint8_t LocoNetCVClass::processLNCVMessage(lnMsg * LnPacket) {
        uint8_t ConsumedFlag(0);

        switch (LnPacket->sr.command) {
        case OPC_IMM_PACKET:
        case OPC_PEER_XFER:
                printf("Possibly a LNCV message.");
                // Either of these message types may be a LNCV message
                // Sanity check: Message length, Verify addresses
                if (LnPacket->ub.mesg_size == 15 && LnPacket->ub.DSTL == LNCV_MODULE_DSTL && LnPacket->ub.DSTH == LNCV_MODULE_DSTH) {
                        // It is a LNCV programming message
                        computeBytesFromPXCT(LnPacket->ub);
                        #ifdef DEBUG_OUTPUT
                        Serial.print("Message bytes: ");
                        Serial.print(LnPacket->ub.ReqId);
                        Serial.write(" ");
                        Serial.print(LnPacket->ub.payload.data.deviceClass, HEX);
                        Serial.write(" ");
                        Serial.print(LnPacket->ub.payload.data.lncvNumber, HEX);
                        Serial.write(" ");
                        Serial.print(LnPacket->ub.payload.data.lncvValue, HEX);
                        Serial.write("\n");
                        #endif

                        lnMsg response;

                        switch (LnPacket->ub.ReqId) {
                        case LNCV_REQID_CFGREQUEST:
                                if (LnPacket->ub.payload.data.deviceClass == 0xFFFF && LnPacket->ub.payload.data.lncvNumber == 0x0000 && LnPacket->ub.payload.data.lncvValue == 0xFFFF) {
                                        // This is a discover message
                                        DEBUG("LNCV discover: ");
                                        if (notifyLNCVdiscover) {
                                                DEBUG(" executing...");
                                                if (notifyLNCVdiscover(LnPacket->ub.payload.data.deviceClass, LnPacket->ub.payload.data.lncvValue) == LNCV_LACK_OK) {
                                                        makeLNCVresponse(response.ub, LnPacket->ub.SRC, LnPacket->ub.payload.data.deviceClass, 0x00, LnPacket->ub.payload.data.lncvValue, 0x00);
                                                        LocoNet.send(&response);
                                                }
                                        }
                                        #ifdef DEBUG_OUTPUT
                                        else {DEBUG(" NOT EXECUTING!");}
                                        #endif
                                } else if (LnPacket->ub.payload.data.flags == 0x00) {
                                        // This can only be a read message
                                        DEBUG("LNCV read: ");
                                        if (notifyLNCVread) {
                                                DEBUG(" executing...");
                                                int8_t returnCode(notifyLNCVread(LnPacket->ub.payload.data.deviceClass, LnPacket->ub.payload.data.lncvNumber, LnPacket->ub.payload.data.lncvValue, LnPacket->ub.payload.data.lncvValue));
                                                if (returnCode == LNCV_LACK_OK) {
                                                        // return the read value
                                                        makeLNCVresponse(response.ub, LnPacket->ub.SRC, LnPacket->ub.payload.data.deviceClass, LnPacket->ub.payload.data.lncvNumber, LnPacket->ub.payload.data.lncvValue, 0x00); // TODO: D7 was 0x80 here, but spec says that it is unused.
                                                        LocoNet.send(&response);        
                                                        ConsumedFlag = 1;
                                                } else if (returnCode >= 0) {
                                                        uint8_t old_opcode(0x7F & LnPacket->ub.command);
                                                        LocoNet.send(OPC_LONG_ACK, old_opcode, returnCode);
                                                        // return a nack
                                                        ConsumedFlag = 1;
                                                }
                                        }
                                        #ifdef DEBUG_OUTPUT
                                        else {DEBUG(" NOT EXECUTING!");}
                                        #endif
                                } else {
                                        // Its a "control" message
                                        DEBUG("LNCV control: ");
                                        if ((LnPacket->ub.payload.data.flags & LNCV_FLAG_PRON) != 0x00 && ((LnPacket->ub.payload.data.flags & LNCV_FLAG_PROFF) != 0x00)) {
                                                DEBUG("Illegal, ignoring.");
                                                // Illegal message, no action.
                                        } else if ((LnPacket->ub.payload.data.flags & LNCV_FLAG_PRON) != 0x00) {
                                                DEBUG("Programming Start, ");
                                                // LNCV PROGAMMING START
                                                // We'll skip the check whether D[2]/D[3] are 0x0000.
                                                if (notifyLNCVprogrammingStart) {
                                                        DEBUG(" executing...");
                                                        if (notifyLNCVprogrammingStart(LnPacket->ub.payload.data.deviceClass, LnPacket->ub.payload.data.lncvValue) == LNCV_LACK_OK) {
                                                                DEBUG("LNCV_LACK_OK ");
                                                                DEBUG(LnPacket->ub.payload.data.deviceClass);
                                                                DEBUG(" ");
                                                                DEBUG(LnPacket->ub.payload.data.lncvValue);
                                                                DEBUG("\n");
                                                                makeLNCVresponse(response.ub, LnPacket->ub.SRC, LnPacket->ub.payload.data.deviceClass, 0x00, LnPacket->ub.payload.data.lncvValue, 0x80);
								ThisThread::sleep_for(10ms); // for whatever reason, we need to delay, otherwise the message will not be sent.
                                                                #ifdef DEBUG_OUTPUT
                                                                printPacket((lnMsg*)&response);
                                                                #endif
                                                                LocoNet.send((lnMsg*)&response);        
                                                                #ifdef DEBUG_OUTPUT
                                                                LN_STATUS status = LocoNet.send((lnMsg*)&response);     
                                                                Serial.print(F("Return Code from Send: "));
                                                                Serial.print(status, HEX);
                                                                Serial.print("\n");
                                                                #endif
                                                                ConsumedFlag = 1;
                                                        } // not for us? then no reaction!
                                                        #ifdef DEBUG_OUTPUT
                                                        else {DEBUG("Ignoring.\n");}
                                                        #endif
                                                } 
                                                #ifdef DEBUG_OUTPUT
                                                else {DEBUG(" NOT EXECUTING!");}
                                                #endif
                                                        
                                        }
                                        if ((LnPacket->ub.payload.data.flags & LNCV_FLAG_PROFF) != 0x00) {
                                                // LNCV PROGRAMMING END
                                                if (notifyLNCVprogrammingStop) {
                                                        notifyLNCVprogrammingStop(LnPacket->ub.payload.data.deviceClass, LnPacket->ub.payload.data.lncvValue);
                                                        ConsumedFlag = 1;
                                                }
                                        }
                                        // Read-Only mode not implmeneted.
                                }

                        break;
                        case LNCV_REQID_CFGWRITE:
                                if (notifyLNCVwrite) {
                                        // Negative return code indicates that we are not interested in this message.
                                        int8_t returnCode(notifyLNCVwrite(LnPacket->ub.payload.data.deviceClass, LnPacket->ub.payload.data.lncvNumber, LnPacket->ub.payload.data.lncvValue));
                                        if (returnCode >= 0) {
                                                ConsumedFlag = 1;
                                                uint8_t old_opcode(0x7F & LnPacket->ub.command);
                                                LocoNet.send(OPC_LONG_ACK, old_opcode, returnCode);
                                        }
                                }
                        break;

                        }

                }
        break;
#ifdef DEBUG_OUTPUT
        default:
                Serial.println("Not a LNCV message."); 
#endif
        }

        return ConsumedFlag;
}

void LocoNetCVClass::makeLNCVresponse( UhlenbrockMsg & ub, uint8_t originalSource, uint16_t first, uint16_t second, uint16_t third, uint8_t last) {
        ub.command = OPC_PEER_XFER;
        ub.mesg_size = 15;
        ub.SRC = LNCV_SRC_MODULE;
        switch (originalSource) {
                case LNCV_SRC_KPU:
                        // Only in case of SRC == 0x01 should something specific be done.
                        ub.DSTL = LNCV_INTELLIBOX_KPU_DSTL;
                        ub.DSTH = LNCV_INTELLIBOX_KPU_DSTH;
                break;
                default:
                        ub.DSTL = originalSource;
                        ub.DSTH = 0x00;
        }
        ub.ReqId = LNCV_REQID_CFGREAD;
        ub.PXCT1 = 0x00;
        ub.payload.data.deviceClass = first;
        ub.payload.data.lncvNumber = second;
        ub.payload.data.lncvValue = third;
        ub.payload.data.flags = last;
        computePXCTFromBytes(ub);
}


void LocoNetCVClass::computeBytesFromPXCT( UhlenbrockMsg & ub) {
        uint8_t mask(0x01);
        // Data has only 7 bytes, so we consider only 7 bits from PXCT1
        for (int i(0); i < 7; ++i) {
        if ((ub.PXCT1 & mask) != 0x00) {
                // Bit was set
                ub.payload.D[i] |= 0x80;
        }
        mask <<= 1;
        }
        ub.PXCT1 = 0x00;
}

void LocoNetCVClass::computePXCTFromBytes( UhlenbrockMsg & ub ) {
        uint8_t mask(0x01);
        ub.PXCT1 = 0x00;
        // Data has only 7 bytes, so we consider only 7 bits from PXCT1
        for (int i(0); i < 7; ++i) {
        if ((ub.payload.D[i] & 0x80) != 0x00) {
                ub.PXCT1 |= mask; // add bit to PXCT1
                ub.payload.D[i] &= 0x7F;        // remove bit from data byte
        }
        mask <<= 1;
        }
}

uint16_t LocoNetCVClass::getAddress(uint8_t lower, uint8_t higher) {
        uint16_t result(higher);
        result <<= 8;
        result |= lower;
        return result;
}
