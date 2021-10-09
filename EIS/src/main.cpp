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
uint8_t recvCount = 0; // counts how many data bytes were received
bool newPacket = false; // raised signal to signify new packet has fully arrived

struct recvMsg{
  uint8_t start;
  uint8_t category;
  uint8_t numData;
  uint8_t data[10];
  uint8_t end;
} newRecvMsg, lastRecvMsg;

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
      Serial.println(client.available());
      /*
      Need to pase through bytes and make sense of them in a state machine
      1. Start Byte: 0x00 
      2. Category Byte: 0xSS
      3. Data Size Byte: 0xNN
      4. Data Bytes: 0xXX
      5. End Byte: 0xFF
      */

      uint8_t incomingByte = client.read();

      // state machine for receiving a packet of data
      switch(recvState)
      {
        case 0:
          if(incomingByte == (uint8_t) 0x00)
            recvState = 1; // transition to sensor byte state
          break;

        case 1:
            newRecvMsg.category = incomingByte;
            recvState = 2; // transition to variable byte state
          break;

        case 2:
          {
            newRecvMsg.numData = incomingByte;
            recvState = 3; // transition to received data byte(s) state
          }
          break;

        case 3:
          if(newRecvMsg.numData <= recvCount)
          {
            recvState = 4; // transition to end byte state
            recvCount = 0;
          }
          else{
            newRecvMsg.data[recvCount] = incomingByte;
            recvCount++;
          }
          break;
        
        case 4:
          if(incomingByte == (uint8_t) 0xFF)
          {
            newRecvMsg.end = incomingByte;
            recvState = 0; // transition to start byte state
            newPacket = true;
            lastRecvMsg = newRecvMsg;
          }
          break;

        default:
          recvState = 0; // transition to start byte state
          break;
      }
    

      // handler for querrying and gathering data to be transmitted
      if(newPacket) {

      }    
    }
 }
}
