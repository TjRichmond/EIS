#include <SPI.h>
#include <Ethernet.h>

// The IP address will be dependent on your local network.
// gateway and subnet are optional:

byte mac[] = {0x53, 0x43, 0x49, 0x4F, 0x4E, 0x32 }; // SCION2 in ASCII

// Static Locate and Remote Addresses
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

// Telemetry Data Port
EthernetServer server(50003); 

bool alreadyConnected = false; // whether or not the client was connected previously

uint8_t recvState = 0; // indicates which byte is being handled from received packet


struct recvMsg{
  uint8_t start;
  uint8_t sensor;
  uint8_t variable;
  uint8_t end;
} lastRecvMsg;

void setup() {
  Ethernet.begin(mac, ip, myDns, gateway, subnet);

  // Open serial communications and wait for port to open:
  Serial.begin(9600);

   while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }

  // Check for Ethernet Cable 
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // Begin listening for clients
  server.begin();
  Serial.print("Telemtry Data Server Address: ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  // Check for any clients
  EthernetClient client = server.available();
  
  if (client) {
    if (!alreadyConnected) {
      // clear out the input buffer:
      client.flush();
      Serial.println("We have a new client");
      Serial.print("The client's IP: ");
      Serial.print(client.remoteIP());
      Serial.print(" Port: ");
      Serial.println(client.remotePort());
      alreadyConnected = true;
    }

    if (client.available() > 0) {
      // read the bytes incoming from the client:

      /*
      Need to pase through bytes and make sense of them in a state machine
      1. Start Byte: 0x00 
      2. Sensor Byte: 0x10-0x30
      3. Variable Byte: 0x40-x60
      4. End Byte: 0xFF
      */

      uint8_t incomingByte = client.read();

      switch(recvState)
      {
        case 0:
          if(incomingByte == (uint8_t) 0x00)
            recvState = 1; // transition to sensor byte state
          break;

        case 1:
          if(incomingByte > (uint8_t) 0x00 && incomingByte < (uint8_t) 0xFF) {
            lastRecvMsg.sensor = incomingByte;
            recvState = 2; // transition to variable byte state
          }
          else {
            recvState = 0;
          }
          break;

        case 2:
          if(incomingByte > (uint8_t) 0x00 && incomingByte < (uint8_t) 0xFF) {
                      lastRecvMsg.variable = incomingByte;
          recvState = 3; // transition to end byte state
          }
          else {
            recvState = 0;
          }
          break;

        case 3:
          if(incomingByte == (uint8_t) 0xFF)
            recvState = 4; // transition to send byte state
          else {
            recvState = 0;
          }
          break;

        default:
          recvState = 0;
          break;
      }
    }

    if(recvState == 4)
    {
      switch(lastRecvMsg.sensor)
      {
        case (uint8_t) 0x10:
          switch(lastRecvMsg.variable)
          {
            case (uint8_t) 0xEE:
              // get variation of senor 0x10's data
              // send it to client
              break;
          }
          break;
        
        case (uint8_t) 0x12:
          switch(lastRecvMsg.variable)
          {
            case (uint8_t) 0xEE:
              // get variation of senor 0x10's data
              // send it to client
              break;
          }
          break;
      }
    }

  }
}