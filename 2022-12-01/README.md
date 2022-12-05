# Robust UDP Protocol

Max on-fly packets: 1,000
Delay time: 50 ms - 150 ms
Loss rate: 40%
Corrupt rate: 10%
Speed limit: 10 Mbps
MTU: 1,500

Consider for 1,000 files with 32 KB size.
Splitted into about 11,500 chunks with 1.5 KB size.

```bash
ip link set dev lo mtu 1500
tc qdisc del dev lo root netem
tc qdisc add dev lo root netem limit 1000 delay 100ms 50ms loss 40% corrupt 10% rate 10Mbit
```

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

For 8,193 - 16,386, it will have 2 chunk groups.
