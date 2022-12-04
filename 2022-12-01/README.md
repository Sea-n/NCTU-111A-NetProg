# Robust UDP Protocol

netem spec: limit 1000 delay 100ms 50ms loss 40% corrupt 10% rate 10Mbit

Max on-fly packets: 1,000
Delay time: 50 ms - 150 ms
Loss rate: 40%
Corrupt rate: 10%
Speed limit: 10 Mbps
MTU: 1,500

Consider for 1,000 files with 32 KB size.
Splitted into about 20,000 chunks with 1.5 KB size.

## RUP Structure
UDP Payload: Packet index, CRC32, Content

## Behavior
- Server: Merge all data into one big packet, then split into chunks
- Client: Send hello for 10 times
- Server: Send first 3 chunks for 10 times
- Server: Start sending all packets for one time
- Client: Send all status updates every 100 ms
- Server: After initial packets, send missing packets in sequence

## RUF structure
- (int) Total chunk count
- (int) File sizes * 1000
- File content * 1000

### Status update
- (int) High bits of this chunk group
- (bit) Received or not * 8192

For 16385 - 24576 chunks, it will have 3 chunk groups.
