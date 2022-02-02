/*
Author: Tristan Richmond<tjrichmond99@gmail.com>
Last Modified: 01/31/21
Description: Function library for handling TCP messages and CAN messages in Mechatronics' EIS 2022 PCB
*/

#ifndef CANTCP_H
#define CANTCP_H

#include <FlexCAN_T4.h>
#include <Ethernet.h>

typedef unsigned char uint8_t;

typedef struct tcpMsg
{
  uint8_t start = 0x00; // start byte
  uint8_t ID;           // byte for identifying the message category
  uint8_t control;      // byte for setting control bits (data length, remote/data)
  uint8_t data[8];      // array for data section of message
  uint8_t end = 0xFF;   // ending byte
} tcpMsg;

void TCPtoCAN(tcpMsg &, CAN_message_t &);
void CANtoTCP(CAN_message_t &, tcpMsg &);
void CANHandler(FlexCAN_T4 <CAN1, RX_SIZE_256, TX_SIZE_16>&, CAN_message_t &, CAN_message_t &);
uint8_t TCPListener(tcpMsg &, tcpMsg &, EthernetClient *, bool *);
void TCPSender(tcpMsg &, EthernetClient *, bool *);

#endif