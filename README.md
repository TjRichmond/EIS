# EIS
Teensy 4.0 + Wiznet 5500 Operated CAN to TCP/IP Converter
Project will contain PCB design and custom firmware

EIS board serves as a TCP Server listening for up to 8 clients to request telemetry data.
EIS is the middle communicator between the sensor's CAN bus and to the TCP/IP network switch

Packet Frame:
Start Byte - Sensor Type Byte - Combination of Sensor Data - End Byte

Firmware is written in C++ using the Arduino Framework and developed in VSCode with the Platform.io extension
