# EIS
Ethernet Interface System

## Initial Prototype Design
**Due to 2021 chip shortage, prototyped version will be used instead of designed PCB**

Teensy 4.0 + Wiznet 5500 Ethernet Interface Module + CAN Transceiver Module

## Final Design's Intentions
Converts CAN Bus Data into Ethernet TCP/IP Data. Used in **SDSU Mechatronic's Robots**

### Packet Frame
| Start Byte | Category Byte | Number of Data Bytes | Data Byte(s) | End Byte |
| --- | --- | --- | --- | --- |
| 0x00 | 0x00 - 0xFF | 0x00 - 0xFF | (N) 0xDD | 0xFF |

Firmware is written in C++ using the Arduino Framework and developed in VSCode with the Platform.io extension
