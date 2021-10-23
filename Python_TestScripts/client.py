import socket

# Configures the IPv4 and Port for CAN-Ethernet Server Interface
HOST = '192.168.55.177'
PORT = 50003

BUFFER = [b'\x00', b'\x00', b'\x80', b'\xFF']

"""
One Packet has the following format:
Start Byte  - ID Byte         - Control Byte    -    Data Byte(s)      - End Byte
Always 0x00 - Full Byte Range - Full Byte Range -    Full Byte Range   - N Byte(s) Full Range - Always 0xFF

Control Bit Format:
R00E LLLL
R: Remote (1) or Data Frame (0)
E: Standard (0) or Extended Frame (1)
L: Number of Data Bytes

Example Data Packet:
Start Byte: b'\x00'
ID Byte: b'\x00'
Control Byte: b'\x03' - Translates to Standard Data Frame with 3 Data Bytes
Data Bytes: b'\xAA' b'\xBB' b'\xCC'
End Byte: b'\xFF'

Example Remote Packet:
Start Byte: b'\x00'
ID Byte: b'\x00'
Control Byte: b'\x80' - Translates to Standard Remote Frame
End Byte: b'\xFF'

If sending a remote frame, Number of Data Bytes is b'\x80'
and the Data Byte Section is omitted making the End Byte section next instead
"""

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    """
    Connects to Teensy Server and sends the entire buffer one byte at a time.
    Then waits to receive any data from the Server with a buffer up to 1024 bits.
    Lastly it will print out the buffer for viewing
    """
    s.connect((HOST, PORT))

    for x in range(len(BUFFER)):
        s.send(BUFFER[x])
    data:bytes = s.recv(1024)
    print('Received From Data: ', repr(data))

for x in range(data[2]+4):
    print(data[x])

