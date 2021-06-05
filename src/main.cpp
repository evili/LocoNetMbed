#include <mbed.h>
#include <LocoNet.h>

#define BAUD_RATE (115200)
BufferedSerial pc(USBTX, USBRX, BAUD_RATE);

volatile bool print_stats = false;

void time_for_stats() {
  print_stats = true;
}

Timeout stats;


int main() {
  //LocoNetClass LocoNet = LocoNetClass();
  // First initialize the LocoNet interface
  LocoNet.init();
  printf("Starting LocoNet Monitor\n");
  
  while(true) {
    // Check for any received LocoNet packets
    lnMsg *LnPacket = LocoNet.receive() ;
    int msgLen = getLnMsgSize(LnPacket);
    
    if(LnPacket) {
      printf("LocoNet Packet Received (%d): %0x.2\n",
	     msgLen,
	     LnPacket->data[0]);
    }
    if(print_stats) {
	    printf("Trying to send...\n");
	    status = LocoNet.reportSwitch(1);
	    printf("Sent status: %s\n", LocoNet.getStatusStr(status));
	    lnStats = LocoNet.getStats();
	    printf("LocoNet Stats:\n");
	    printf("  Received: %u/%u\n", lnStats->RxPackets, lnStats->RxErrors);
	    printf("  Transmitted: %u/%u\n", lnStats->TxPackets, lnStats->TxErrors);
	    printf("  Collisions: %u\n\n", lnStats->Collisions);
	    print_stats = false;
	    stats.attach(time_for_stats, 5s);
    }
  } 
}
