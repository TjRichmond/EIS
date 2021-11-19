# EIS
Ethernet Interface System

## Initial Prototype Design
**Due to 2021 chip shortage, prototyped version will be used instead of designed PCB**

Teensy 4.0 + Wiznet 5500 Ethernet Interface Module + CAN Transceiver Module

## Final Design's Intentions
Converts CAN Bus Data into Ethernet TCP/IP Data. Used in **SDSU Mechatronic's Robots**

### Packet Frame
| Start Byte | Control Byte | Data Byte(s) | End Byte |
| --- | --- | --- | --- |
| 0x00 | R00E LLLL | 0x00 - 0xFF | (N) 0xDD | 0xFF |

Control Byte contains fields for frame descriptions
- Remote Frame (R)
- Extend/Standard CAN Frame (E)
- Data Length of Frame (L)

Firmware is written in C++ using the Arduino Framework and developed in VSCode with the Platform.io extension
