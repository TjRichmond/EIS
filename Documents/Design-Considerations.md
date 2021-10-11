# Design Considerations
## PCB
Designed in Altium Designer Software

| Component | Description|
| ----------- | ----------- |
| STM32F4107 | Microcontroller|
| DP839848 | Ethernet Phys Interface Controller | 
| MCP2551 | CAN Bus Transceiver IC |

| Protocol Interface | Connector Name |
| ----------- | ------------ |
| Ethernet | RJ45 Connector |
| CAN Bus | 2-pin Terminal Block |
| USART | 2-pin Header Pin |
| I2C | 4-pin Hirose Connector |
| SPI | 4-pin Hirose Connector |
| Analog | 2-pin Header Pin |

## Firmware
Written with HAL framework inside of STM32Cube IDE

## Functionalty
Plug and play module to interface any signal into ethernet base one hardware mode jumper or switch
