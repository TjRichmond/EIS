#include <SPI.h>
#include <Ethernet.h>
#include <FlexCAN_T4.h>

/* TODO
Interrupt Timers for each Sensor PCB to request a pulse
Look into DMA for Ethernet to SPI
CAN DMA?
*/

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> myCan;

CAN_message_t canTXMsg, canRXMsg;

// The IP address will be dependent on your local network.
// gateway and subnet are optional:

byte mac[] = {0x53, 0x43, 0x49, 0x4F, 0x4E, 0x32}; // SCION2 in ASCII

// Static Locate and Remote Addresses
IPAddress ip(192, 168, 55, 177);
IPAddress myDns(192, 168, 55, 1);
IPAddress gateway(192, 168, 55, 1);
IPAddress subnet(255, 255, 0, 0);

// Telemetry Data Port
EthernetServer server(50003);

bool alreadyConnected = false; // whether or not the client was connected previously

uint8_t recvState = 0;  // indicates which byte is being handled from received packet
uint8_t recvCount = 0;  // counts how many data bytes were received
bool newPacket = false; // raised signal to signify new packet has fully arrived

uint8_t errorCode = 0;

struct tcpMsg
{
  uint8_t start;
  uint8_t ID;
  uint8_t control;
  uint8_t data[8];
  uint8_t end;
} newRecvMsg, lastRecvMsg, tcpTXMsg;

void TCPtoCAN(tcpMsg &, CAN_message_t &);
void CANtoTCP(CAN_message_t &, tcpMsg &);
void CANHandler(CAN_message_t &, CAN_message_t &);
void TCPListener(tcpMsg &, tcpMsg &, EthernetClient *, bool *);
void TCPSender(tcpMsg &, EthernetClient *, bool *);
void errorHandler(uint8_t *, EthernetClient *);

void setup()
{
  myCan.begin();
  myCan.setBaudRate(1000000);

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
  // Check for any clients
  EthernetClient client = server.available();

  if (client)
  {
    if (!alreadyConnected)
    {
      // clear out the input buffer:
      client.flush();
      Serial.println("We have a new client");
      Serial.print("The client's IP: ");
      Serial.print(client.remoteIP());
      Serial.print(" Port: ");
      Serial.println(client.remotePort());
      alreadyConnected = true;
    }

    TCPListener(newRecvMsg, lastRecvMsg, &client, &newPacket);
    // handler for querrying and gathering data to be transmitted

    if (newPacket)
    {
      TCPtoCAN(lastRecvMsg, canRXMsg);
      CANHandler(canRXMsg, canTXMsg);
      CANtoTCP(canTXMsg, tcpTXMsg);
      TCPSender(tcpTXMsg, &client, &newPacket);
    }

    if (errorCode > 0)
    {
      errorHandler(&errorCode, &client);
      Serial.println(errorCode, HEX);
    }
  }
}

void TCPtoCAN(tcpMsg &tcpPacket, CAN_message_t &canFrame)
{
  // Function that converts tcpPack into Can Frame
  canFrame.id = tcpPacket.ID;
  canFrame.len = (tcpPacket.control & 0x0F);
  canFrame.flags.extended = (tcpPacket.control & 0x10);
  canFrame.flags.remote = (tcpPacket.control & 0x80);
  int i;
  for (i = 0; i < canFrame.len; i++)
  {
    canFrame.buf[i] = tcpPacket.data[i];
  }
}

void CANtoTCP(CAN_message_t &canFrame, tcpMsg &tcpPacket)
{
  tcpPacket.ID = canFrame.id;
  tcpPacket.control = tcpPacket.control | canFrame.len;
  tcpPacket.control = tcpPacket.control | (canFrame.flags.extended << 4);
  tcpPacket.control = tcpPacket.control | (canFrame.flags.remote << 7);
  int i;
  for (i = 0; i < canFrame.len; i++)
  {
    tcpPacket.data[i] = canFrame.buf[i];
  }
}

void CANHandler(CAN_message_t &canRXFrame, CAN_message_t &canTXFrame)
{
  uint8_t canState = 0;
  // Replace canRXMsg with Argument
  switch (canState)
  {
  case 0:
    if (myCan.write(canTXFrame))
    {
      Serial.println("Sent message");
      Serial.print("CAN1 ");
      Serial.print("MB: ");
      Serial.print(canTXFrame.mb);
      Serial.print("  ID: 0x");
      Serial.print(canTXFrame.id, HEX);
      Serial.print("  EXT: ");
      Serial.print(canTXFrame.flags.extended);
      Serial.print("  LEN: ");
      Serial.print(canTXFrame.len);
      Serial.print(" DATA: ");
      for (uint8_t i = 0; i < canTXFrame.len; i++)
      {
        Serial.print(canTXFrame.buf[i]);
        Serial.print(" ");
      }
      Serial.print("  TS: ");
      Serial.println(canTXFrame.timestamp);
      canState = 1;
    }
    break;

  case 1:
    if (myCan.read(canRXFrame))
    {
      Serial.println("Received Message");
      Serial.print("CAN1 ");
      Serial.print("MB: ");
      Serial.print(canRXFrame.mb);
      Serial.print("  ID: 0x");
      Serial.print(canRXFrame.id, HEX);
      Serial.print("  EXT: ");
      Serial.print(canRXFrame.flags.extended);
      Serial.print("  LEN: ");
      Serial.print(canRXFrame.len);
      Serial.print(" DATA: ");
      for (uint8_t i = 0; i < canRXFrame.len; i++)
      {
        Serial.print(canRXFrame.buf[i]);
        Serial.print(" ");
      }
      Serial.print("  TS: ");
      Serial.println(canRXFrame.timestamp);
      if (canRXFrame.id == 0x00)
      {
        canState = 2;
      }
    }
    break;

  default:
    break;
  }
}

void TCPListener(tcpMsg &newPacket, tcpMsg &lastPacket, EthernetClient *client, bool *doneFlag)
{

  uint8_t rxBytesNum = client->available();
  if (rxBytesNum > 0)
  {

    /*
      Need to pase through bytes and make sense of them in a state machine
      1. Start Byte: 0x00
      2. ID Byte: 0xII
      3. Control Byte: 0xRL
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
      newPacket.ID = incomingByte;
      recvState = 2; // transition to variable byte state
      Serial.println("ID");
      break;

    case 2:
    {
      newPacket.control = incomingByte;

      if (newPacket.control & 0x80)
      {
        if (newPacket.control & 0x10)
        {
          // Extended Frame Remote Command
        }
        else if ((newPacket.control & 0x10) == 0x00)
        {
          // Standard Frame Remote Command
          recvState = 4;
          Serial.println("Remote Frame Byte");
        }
      }
      else if ((newPacket.control & 0x80) == 0x00)
      {
        if (newPacket.control & 0x10)
        {
          // Extended Frame Data Command
        }
        else if ((newPacket.control & 0x10) == 0x00)
        {
          // Standard Frame Data Command
          recvState = 3; // transition to received data byte(s) state
          Serial.print("Number of Data Bytes: ");
          Serial.println(newPacket.control);
        }
      }
    }
    break;

    case 3:
      if ((newPacket.control & 0x0F) - 1 <= recvCount)
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

void TCPSender(tcpMsg &packet, EthernetClient *client, bool *newPacket)
{
  uint8_t dataLen = packet.control & 0x0F;
  uint8_t *sendBuf = new uint8_t;
  sendBuf[0] = packet.start;
  sendBuf[1] = packet.ID;
  sendBuf[2] = packet.control;
  if (dataLen > 0)
  {
    int i;
    for (i = 0; i < dataLen; i++)
    {
      sendBuf[3 + i] = packet.data[i];
    }
    sendBuf[4 + i] = packet.end;
  }
  else
  {
    sendBuf[3] = packet.end;
  }
  client->write(sendBuf, (size_t)(dataLen + 4));
  delete sendBuf;

  // switch (packet.ID)
  // {
  // case 0x00:
  //   // Check Alive Status of EPS
  //   Serial.println("Check alive status of EPS");
  //   client->print("Check alive status of EPS");
  //   break;
  // case 0x01:
  //   // Retrieve all data from EPS
  //   Serial.println("Retrieve all EPS data");
  //   client->print("EPS Data");
  //   break;
  // default:
  //   Serial.println("Command Byte Unable to Decode");
  //   break;
  // }

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
