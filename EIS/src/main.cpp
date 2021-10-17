#include <SPI.h>
#include <Ethernet.h>
#include <FlexCAN_T4.h>

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> myCan;

CAN_message_t canMsg;

// The IP address will be dependent on your local network.
// gateway and subnet are optional:

byte mac[] = {0x53, 0x43, 0x49, 0x4F, 0x4E, 0x32}; // SCION2 in ASCII

// Static Locate and Remote Addresses
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

// Telemetry Data Port
EthernetServer server(50003);

bool alreadyConnected = false; // whether or not the client was connected previously

uint8_t recvState = 0;  // indicates which byte is being handled from received packet
uint8_t recvCount = 0;  // counts how many data bytes were received
bool newPacket = false; // raised signal to signify new packet has fully arrived

uint8_t canState = 0;

uint8_t errorCode = 0;

struct msg
{
  uint8_t start;
  uint8_t category;
  uint8_t numData;
  uint8_t data[10];
  uint8_t end;
} newRecvMsg, lastRecvMsg;

void CANHandler(void);
void TCPHandler(msg &, msg &, EthernetClient *, bool *);
void packetDecoder(msg &, EthernetClient *, bool *);
void errorHandler(uint8_t *, EthernetClient *);

void setup()
{
  myCan.begin();
  myCan.setBaudRate(1000000);

  canMsg.id = 0x1;

  uint8_t data[1] = {23};

  canMsg.buf[0] = data[0];

  canMsg.len = 1;

  // Establish Ethernet Server with Static IP
  Ethernet.begin(mac, ip, myDns, gateway, subnet);

  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  // while (!Serial)
  // {
  //   ; // wait for serial port to connect. Needed for native USB port only
  // }

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    Serial.println("Ethernet shield was not found.");
    while (true)
    {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }

  // Check for Ethernet Cable
  if (Ethernet.linkStatus() == LinkOFF)
  {
    Serial.println("Ethernet cable is not connected.");
  }

  // Begin listening for clients
  server.begin();
  Serial.print("Telemtry Data Server Address: ");
  Serial.println(Ethernet.localIP());
}

void loop()
{
  CANHandler();

  // // Check for any clients
  // EthernetClient client = server.available();

  // if (client)
  // {
  //   if (!alreadyConnected)
  //   {
  //     // clear out the input buffer:
  //     client.flush();
  //     Serial.println("We have a new client");
  //     Serial.print("The client's IP: ");
  //     Serial.print(client.remoteIP());
  //     Serial.print(" Port: ");
  //     Serial.println(client.remotePort());
  //     alreadyConnected = true;
  //   }

  //   TCPHandler(newRecvMsg, lastRecvMsg, &client, &newPacket);
  //   // handler for querrying and gathering data to be transmitted

  //   if (newPacket)
  //   {
  //     packetDecoder(lastRecvMsg, &client, &newPacket);
  //   }

  //   if (errorCode > 0)
  //   {
  //     errorHandler(&errorCode, &client);
  //     Serial.println(errorCode, HEX);
  //   }
  // }
}

void CANHandler()
{
  switch (canState)
  {
  case 0:
    if (myCan.write(canMsg))
    {
      Serial.println("Sent message");
      Serial.print("CAN1 ");
      Serial.print("MB: ");
      Serial.print(canMsg.mb);
      Serial.print("  ID: 0x");
      Serial.print(canMsg.id, HEX);
      Serial.print("  EXT: ");
      Serial.print(canMsg.flags.extended);
      Serial.print("  LEN: ");
      Serial.print(canMsg.len);
      Serial.print(" DATA: ");
      for (uint8_t i = 0; i < canMsg.len; i++)
      {
        Serial.print(canMsg.buf[i]);
        Serial.print(" ");
      }
      Serial.print("  TS: ");
      Serial.println(canMsg.timestamp);
      canState = 1;
    }
    break;

  case 1:
    if (myCan.read(canMsg))
    {
      Serial.println("Received Message");
      Serial.print("CAN1 ");
      Serial.print("MB: ");
      Serial.print(canMsg.mb);
      Serial.print("  ID: 0x");
      Serial.print(canMsg.id, HEX);
      Serial.print("  EXT: ");
      Serial.print(canMsg.flags.extended);
      Serial.print("  LEN: ");
      Serial.print(canMsg.len);
      Serial.print(" DATA: ");
      for (uint8_t i = 0; i < canMsg.len; i++)
      {
        Serial.print(canMsg.buf[i]);
        Serial.print(" ");
      }
      Serial.print("  TS: ");
      Serial.println(canMsg.timestamp);
      if (canMsg.id == 0xAA)
      {
        canState = 2;
      }
    }
    break;
    
  default:
    break;
  }
}

void TCPHandler(msg &newPacket, msg &lastPacket, EthernetClient *client, bool *doneFlag)
{

  uint8_t rxBytesNum = client->available();
  if (rxBytesNum > 0)
  {

    /*
      Need to pase through bytes and make sense of them in a state machine
      1. Start Byte: 0x00 
      2. Category Byte: 0xSS
      3. Data Size Byte: 0xNN
      4. Data Bytes: 0xXX
      5. End Byte: 0xFF
      */

    uint8_t incomingByte = client->read();
    // state machine for receiving a packet of data
    switch (recvState)
    {
    case 0:
      if (incomingByte == (uint8_t)0x00)
      {
        newPacket.start = incomingByte;
        recvState = 1; // transition to sensor byte state
        Serial.println("Start");
      }
      else
      {
        errorCode = 0x02;
      }
      break;

    case 1:
      newPacket.category = incomingByte;
      recvState = 2; // transition to variable byte state
      Serial.println("Command");
      break;

    case 2:
    {
      newPacket.numData = incomingByte;
      recvState = 3; // transition to received data byte(s) state
      Serial.print("Number of Data Bytes: ");
      Serial.println(newPacket.numData);
    }
    break;

    case 3:
      if (newPacket.numData - 1 <= recvCount)
      {
        newPacket.data[recvCount] = incomingByte;
        Serial.print("Last Data Byte Section: ");
        Serial.println((recvCount + 1));
        if (rxBytesNum < 2)
        {
          recvState = 0;
          errorCode = 0x10;
        }
        else
        {
          recvState = 4; // transition to end byte state
        }

        recvCount = 0;
      }
      else
      {
        newPacket.data[recvCount] = incomingByte;
        recvCount++;
        Serial.print("Data Byte Section: ");
        Serial.println(recvCount);
      }
      break;

    case 4:
      if (incomingByte == (uint8_t)0xFF)
      {
        newPacket.end = incomingByte;
        recvState = 0; // transition to start byte state
        *doneFlag = true;
        lastRecvMsg = newPacket;
        Serial.println("End");
      }
      break;

    default:
      recvState = 0; // transition to start byte state
      break;
    }
  }
}

void packetDecoder(msg &packet, EthernetClient *client, bool *newPacket)
{
  switch (packet.category)
  {
  case 0x00:
    // Check Alive Status of EPS
    Serial.println("Check alive status of EPS");
    client->print("Check alive status of EPS");
    break;
  case 0x01:
    // Retrieve all data from EPS
    Serial.println("Retrieve all EPS data");
    client->print("EPS Data");
    break;
  default:
    Serial.println("Command Byte Unable to Decode");
    break;
  }

  Serial.println("Print the Received Data");
  for (int i = 0; i < packet.numData; i++)
    Serial.println(packet.data[i], HEX);

  *newPacket = false;
}

void errorHandler(uint8_t *code, EthernetClient *client)
{
  /*
   Function will be called to return any error cases.
   The error code with a description will be printed to the serial and client
  */
  switch (*code)
  {
  case 0x02:
    client->print("Error Code: 0x02 - Didn't Receive Start Byte '0x00'");
    Serial.println("Error Code: 0x02 - Didn't Receive Start Byte '0x00'");
    break;

  case 0x10:
    client->print("Error Code: 0x10 - Number of Data Bytes Oversize");
    Serial.println("Error Code: 0x10 - Number of Data Bytes Oversize");
    break;
  }
  *code = 0;
}
