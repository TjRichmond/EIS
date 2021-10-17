#include <mbed.h>

int main() {

CAN can(PA_11, PA_12, 1000000);

char data[] = "Hello";

// CANMessage msg;
// msg.id = 127;
// msg.data = data;
// msg.len = 1;

  while(1) {
    can.write(CANMessage(127,data,8));
  }
}