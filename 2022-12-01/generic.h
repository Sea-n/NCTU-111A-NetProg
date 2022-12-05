#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <bitset>
#include <cstdio>

#define MTU (1500 - 28)
#define CHUNK (MTU - 8)  // int * 2

using namespace std;

typedef struct {
	unsigned crc32;
	int index;
	char content[CHUNK] = {};
} RUP;  // Protocol

typedef struct {
	int chunks;
	int size[1000] = {};
	char content[];
} RUF;  // File

unsigned CRC32_TABLE[256] = {};

static unsigned crc32(const RUP* pkt) {
	unsigned char* buf = (unsigned char*) pkt;
	unsigned crc = 0xFFFFFFFF;

	if (!CRC32_TABLE[0]) {
		for (unsigned i=0; i<256; i++) {
			unsigned c = i;
			for (int j=0; j<8; j++)
				if (c & 1)
					c = 0xEDB88320 ^ (c >> 1);
				else
					c >>= 1;
			CRC32_TABLE[i] = c;
		}
	}

	for (int i=4; i<MTU; i++)
		crc = CRC32_TABLE[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
	return crc ^ 0xFFFFFFFF;
}
