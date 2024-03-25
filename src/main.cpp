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
  int power = 1;
  stats.attach(time_for_stats, 5s);
  // First initialize the LocoNet interface
  LocoNet.init(D7);
  printf("Starting LocoNet Monitor\n");
  
  while(true) {
    // Check for any received LocoNet packets
    lnMsg *LnPacket = LocoNet.receive() ;
    int msgLen = getLnMsgSize(LnPacket);
    
    if(LnPacket) {
      printf("LocoNet Packet Received (%d): %02x",
	     msgLen,
	     LnPacket->data[0]);
      for(int i=1; i<msgLen; i++)
	      printf(":%02x", LnPacket->data[i]);
      printf("\n");
    }
    if(print_stats) {
	    printf("Trying to send power ON/OFF  = %d\n", power);
	    LN_STATUS status = LocoNet.reportPower(power);
        printf("Result of sending: %s\n", LocoNet.getStatusStr(status));
	    LnBufStats *lnStats = LocoNet.getStats();
	    printf("LocoNet Stats:\n");
	    printf("  Received: %u/%u\n", lnStats->RxPackets, lnStats->RxErrors);
	    printf("  Transmitted: %u/%u\n", lnStats->TxPackets, lnStats->TxErrors);
	    printf("  Collisions: %u\n\n", lnStats->Collisions);
	    print_stats = false;
        power = 1 - power;
	    stats.attach(time_for_stats, 10s);
    }
  } 
}
