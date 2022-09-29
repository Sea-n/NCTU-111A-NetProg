#!/usr/bin/env bash

if [[ "$(id -u)" -ne 0 ]]; then
	echo "Usage: sudo $0" >&2
	exit 1
fi

pcap="$(mktemp --suffix=.pcap)"
dec="$(mktemp)"
chmod 666 "$pcap"

tcpdump -w "$pcap" 'ip and ip[8] > 140' &
pid1="$!"
nc inp111.zoolab.org 10001 > /dev/null &
pid2="$!"
wait "$pid2"
kill -SIGINT "$pid1"
wait

echo
ls -l "$pcap"
echo

tcpdump -Xr "$pcap" \
| tail -n +2 \
| cut -c52- \
| tr -d '\n' \
| grep -oP '[a-zA-Z0-9/+=]{20,}' \
| tee "$dec"

echo

cat "$dec" \
| base64 -d \
| grep -oP 'FLAG{.*}'

if [[ $? -ne 0 ]]; then
	echo
	tail -c +2 "$dec" \
	| base64 -d \
	| grep -oP 'FLAG{.*}'
fi
