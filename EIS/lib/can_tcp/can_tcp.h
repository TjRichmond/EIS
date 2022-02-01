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
  uint8_t start = 0x00;
  uint8_t ID;
  uint8_t control;
  uint8_t data[8];
  uint8_t end = 0xFF;
} tcpMsg;

void TCPtoCAN(tcpMsg &, CAN_message_t &);
void CANtoTCP(CAN_message_t &, tcpMsg &);
void CANHandler(FlexCAN_T4 <CAN1, RX_SIZE_256, TX_SIZE_16>&, CAN_message_t &, CAN_message_t &);
uint8_t TCPListener(tcpMsg &, tcpMsg &, EthernetClient *, bool *);
void TCPSender(tcpMsg &, EthernetClient *, bool *);

#endif