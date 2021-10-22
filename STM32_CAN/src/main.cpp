#include "mbed.h"

// main() runs in its own thread in the OS
int main() {

  CAN can(PA_11, PA_12, 1000000);

  char data[1] = {0xEE};

  CANMessage msg;

  uint8_t canState = 0;

  while (1) {

    switch (canState) {
    case 0:
      if (can.read(msg)) {
        if (msg.id == 1) {
          printf("Received correct CAN request\n\r");

          for (uint8_t i = 0; i < msg.len; i++) {
            printf("%d ", msg.data[i]);
          }
          printf("\n\r");

          msg.id = 0x00;
          msg.data[0] = data[0];
          msg.len = 1;
          canState = 1;
        }

        break;

      case 1:
        if (can.write(msg)) {
          printf("Sent CAN Message\n\r");
          for (uint8_t i = 0; i < msg.len; i++) {
            printf("%d ", msg.data[i]);
          }
          canState = 2;
        }
        break;

      default:
        break;
      }
    }
  }
}
