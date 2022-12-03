# Robust UDP Protocol

netem spec: limit 1000 delay 100ms 50ms loss 40% corrupt 10% rate 10Mbit

Max on-fly packets: 1,000
Delay time: 50 ms - 150 ms
Loss rate: 40%
Corrupt rate: 10%
Speed limit: 10 Mbps

Consider for 1,000 files with 32 KiB size.

## RUP Structure
UDP Payload: Type, Packet index, Frag count, Frag seq, CRC32, Content
Maximum content size per packet: 1024

### RUF File
General: index, file size, CRC32 of content, filename length, filename
Client: incomplete chunks

## Types
- Type 0x01: Ping
- Type 0x11: Pong
- Type 0x02: Get directory list
- Type 0x12: Directory list
- Type 0x03: Status update
- Type 0x13: Ack to update
- Type 0x14: File content

### Ping / Pong
Request: 128 bytes of random value
Response: 128 bytes of the same value

### Directory list
Request: empty
Response: file count, (index, file size, CRC32 of content, filename length, filename) * N
filename: end with NULL

### Status update
Request: list length, (file index) * N
Response: the same content

The list content all ID of incomplete files.

Server behavior:
- Send response for 8 times
- Set client entry

### File content
Request: not applicable
Response: file index, file size, CRC32 of file, filename length, filename, content

The server will pick files from incomplete pool (sequentially / randomly / by size).

Server behavior:
- Send each packet for 4 times (94% success rate)

## Client behavior
- Send **Type 0x02 Get list** for 8 times (99.6% success rate)
- Construct file structure
- Send first status update for 8 times
- Receive files
- Send status update every 100 ms
