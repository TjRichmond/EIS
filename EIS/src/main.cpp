/*
Author: Tristan Richmond<tjrichmond99@gmail.com>
Last Modified: 01/31/21
Description: EIS embedded software for converting CAN BUS messages into TCP for any SBC to interface. 
Not all SBC have CAN interfaces, but most have an ethernet port
*/

#include <SPI.h>
#include <can_tcp.h>

void errorHandler(uint8_t *, EthernetClient *);

/* TODO
Interrupt Timers for each Sensor PCB to request a pulse
Look into DMA for Ethernet to SPI
CAN DMA?
*/

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> myCan; // CAN BUS Object

CAN_message_t canTXMsg, canRXMsg; // CAN Message Objects

tcpMsg newRecvMsg, lastRecvMsg, tcpTXMsg; // TCP Message Objects

// The IP address will be dependent on your local network.
// gateway and subnet are optional:

byte mac[] = {0x53, 0x43, 0x49, 0x4F, 0x4E, 0x32}; // SCION2 in ASCII

// Static Locate and Remote Addresses
IPAddress ip(192, 168, 55, 177);
IPAddress myDns(192, 168, 55, 1);
IPAddress gateway(192, 168, 55, 1);
IPAddress subnet(255, 255, 255, 0);

// Telemetry Data Port
EthernetServer server(50003);

bool alreadyConnected = false; // whether or not the client was connected previously

bool newPacket = false; // raised signal to signify new packet has fully arrived

uint8_t errorCode = 0; // holds returned errors from functions

void setup()
{
  // Initialize CAN BUS I/O and baud rate
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

    errorCode = TCPListener(newRecvMsg, lastRecvMsg, &client, &newPacket);
    // handler for querrying and gathering data to be transmitted

    if (newPacket)
    {
      TCPtoCAN(lastRecvMsg, canRXMsg);
      CANHandler(myCan, canRXMsg, canTXMsg);
      CANtoTCP(canRXMsg, tcpTXMsg);
      TCPSender(tcpTXMsg, &client, &newPacket);
    }

    if (errorCode > 0)
    {
      errorHandler(&errorCode, &client);
      Serial.println(errorCode, HEX);
    }
  }
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