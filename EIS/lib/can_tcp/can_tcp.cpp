/*
Author: Tristan Richmond<tjrichmond99@gmail.com>
Last Modified: 01/31/21
Description: Function library for handling TCP messages and CAN messages in Mechatronics' EIS 2022 PCB
*/

#include <can_tcp.h>

void TCPtoCAN(tcpMsg &tcpPacket, CAN_message_t &canFrame)
{
    /*
    TCPtoCAN converts a TCP message frame and reformats it to a CAN message frame

    :param tcpPacket: Reference to the TCP packet to import the CAN fields into
    :param canFrame: Reference to given CAN message frame
    */

    canFrame.id = tcpPacket.ID;
    canFrame.len = (tcpPacket.control & 0x0F);
    canFrame.flags.extended = (tcpPacket.control & 0x10);
    canFrame.flags.remote = (tcpPacket.control & 0x80);

    for (int i = 0; i < canFrame.len; i++)
    {
        canFrame.buf[i] = tcpPacket.data[i];
    }
}

void CANtoTCP(CAN_message_t &canFrame, tcpMsg &tcpPacket)
{
    /*
    CANtoTCP converts a CAN message frame and reformats it to a TCP message frame

    :param canFrame: Reference to given CAN message frame
    :param tcpPacket: Reference to the TCP packet to import the CAN fields into
    */

    tcpPacket.ID = canFrame.id;
    tcpPacket.control |= canFrame.len;
    tcpPacket.control |= (canFrame.flags.extended << 4);
    tcpPacket.control |= (canFrame.flags.remote << 7);

    for (int i = 0; i < canFrame.len; i++)
    {
        tcpPacket.data[i] = canFrame.buf[i];
    }
}

void CANHandler(FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> &canBus, CAN_message_t &canRXFrame, CAN_message_t &canTXFrame)
{
    /*
    CANHandler writes a messsage to the CAN BUS and waits to listen for a CAN response

    :param canBus: Reference to CAN BUS I/O
    :param canRXFrame: Reference to object for holding received CAN message
    :param canTXFrame: Reference to object for holding transmited CAN message
    */

    uint8_t canState = 0;
    while (canState != 2)
    {
        switch (canState)
        {
        case 0:
            if (canBus.write(canTXFrame))
            {
                // Serial.println("Sent message");
                // Serial.print("CAN1 ");
                // Serial.print("MB: ");
                // Serial.print(canTXFrame.mb);
                // Serial.print("  ID: 0x");
                // Serial.print(canTXFrame.id, HEX);
                // Serial.print("  EXT: ");
                // Serial.print(canTXFrame.flags.extended);
                // Serial.print("  LEN: ");
                // Serial.print(canTXFrame.len);
                // Serial.print(" DATA: ");
                // for (uint8_t i = 0; i < canTXFrame.len; i++)
                // {
                // Serial.print(canTXFrame.buf[i]);
                // Serial.print(" ");
                // }
                // Serial.print("  TS: ");
                // Serial.println(canTXFrame.timestamp);
                canState = 1;
            }
            break;

        case 1:
            if (canBus.read(canRXFrame))
            {
                // Serial.println("Received Message");
                // Serial.print("CAN1 ");
                // Serial.print("MB: ");
                // Serial.print(canRXFrame.mb);
                // Serial.print("  ID: 0x");
                // Serial.print(canRXFrame.id, HEX);
                // Serial.print("  EXT: ");
                // Serial.print(canRXFrame.flags.extended);
                // Serial.print("  LEN: ");
                // Serial.print(canRXFrame.len);
                // Serial.print(" DATA: ");
                // for (uint8_t i = 0; i < canRXFrame.len; i++)
                // {
                // Serial.print(canRXFrame.buf[i]);
                // Serial.print(" ");
                // }
                // Serial.print("  TS: ");
                // Serial.println(canRXFrame.timestamp);
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
}

uint8_t TCPListener(tcpMsg &newPacket, tcpMsg &lastPacket, EthernetClient *client, bool *doneFlag)
{
    /*
    TCPListener waits checks for incoming TCP packets and stores them

    :param newPacket: Reference to object to store most recent incoming TCP packet
    :param lastPacket: Reference to object for last received TCP packet
    :param client: Reference to socket connection client
    :param doneFlag: Reference to signal for TCP recieve completion

    :return error:
    0x10 - Number of Data Bytes Oversize
    0x02 - Didn't Receive Start Byte '0x00'
    0x00 - No errors
    */

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

        uint8_t recvState = 0; // indicates which byte is being handled from received packet

        uint8_t recvCount = 0; // counts how many data bytes were received

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
                return 0x02;
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
                    return 0x10;
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
                lastPacket = newPacket;
                Serial.println("End");
            }
            break;

        default:
            recvState = 0; // transition to start byte state
            break;
        }
    }

    return 0x00;
}

void TCPSender(tcpMsg &packet, EthernetClient *client, bool *newPacket)
{
    /*
    TCPSender sends a provided TCP message to the client over its socket

    :param packet: Reference to object to store outgoing TCP packet
    :param client: Reference to socket connection client
    :param newPacket: Reference to signal for TCP send completion
    */

    uint8_t dataLen = packet.control & 0x0F;
    uint8_t *sendBuf = new uint8_t;

    sendBuf[0] = packet.start;
    sendBuf[1] = packet.ID;
    sendBuf[2] = packet.control;

    // Serial.print("Packet Control Byte: ");
    // Serial.println(packet.control, HEX);
    if (dataLen > 0)
    {
        int i;
        for (i = 0; i < dataLen; i++)
        {
            sendBuf[3 + i] = packet.data[i];
        }
        sendBuf[3 + i] = packet.end;
    }
    else
    {
        sendBuf[3] = packet.end;
    }
    // Serial.print("Packet End Byte: ");
    // Serial.println(packet.end, HEX);

    client->write(sendBuf, (size_t)(dataLen + 4));

    delete sendBuf;

    *newPacket = false;
}