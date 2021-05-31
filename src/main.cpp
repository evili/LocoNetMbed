#include <mbed.h>
#include <LocoNet.h>

#define BAUD_RATE (115200)
BufferedSerial pc(USBTX, USBRX, BAUD_RATE);


int main() {
  LocoNetClass LocoNet = LocoNetClass();
  lnMsg        *LnPacket;
  uint8_t msgLen;

  // First initialize the LocoNet interface
  LocoNet.init();
  printf("Starting LocoNet Monitor\n");
  
  while(true) {
    // Check for any received LocoNet packets
    LnPacket = LocoNet.receive() ;
    msgLen = getLnMsgSize(LnPacket);
    
    if(LnPacket) {
      printf("LocoNet Packet Received (%d): %0x.2\n",
	     msgLen,
	     LnPacket->data[0]);
    }
  } 
}
