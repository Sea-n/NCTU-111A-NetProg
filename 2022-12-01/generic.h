#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define MTU 1400
#define CHUNK (MTU - 20)  // int * 5

enum TYPE {
	TYPE_PING = 0x01,
	TYPE_PONG = 0x11,
	TYPE_DIR_LIST_GET = 0x02,
	TYPE_DIR_LIST = 0x12,
	TYPE_STATUS_UPDATE = 0x03,
	TYPE_STATUS_UPDATE_ACK = 0x13,
	TYPE_FILE_CONTENT = 0x14,
	TYPE_UNKNOWN = 0xFF
};

typedef struct {
	int type;
	int index;
	int frag_cnt;
	int frag_seq;
	int crc32;
	char content[CHUNK];
} RUP;

typedef struct {
	int index;
	int file_size;
	int crc32;
	int name_size;
	char filename[256];
} RUF;

struct crc32 {
	static void generate_table(unsigned (&table)[256]) {
		unsigned polynomial = 0xEDB88320;
		for (unsigned i = 0; i < 256; i++) {
			unsigned c = i;
			for (int j = 0; j < 8; j++)
				if (c & 1)
					c = polynomial ^ (c >> 1);
				else
					c >>= 1;
			table[i] = c;
		}
	}

	static unsigned update(unsigned (&table)[256], unsigned initial, const void* buf, int len) {
		unsigned c = initial ^ 0xFFFFFFFF;
		const unsigned char* u = static_cast<const unsigned char*>(buf);
		for (int i = 0; i < len; ++i)
			c = table[(c ^ u[i]) & 0xFF] ^ (c >> 8);
		return c ^ 0xFFFFFFFF;
	}
};

unsigned crc32_calc(RUP *p) {
	p->crc32 = 0;
	unsigned table[256];
	crc32::generate_table(table);
	unsigned crc = crc32::update(table, 0, p, MTU);
	p->crc32 = crc;
	return crc;
}

bool crc32_vrfy(RUP *p) {
	unsigned crc_old = p->crc32;
	p->crc32 = 0;
	unsigned table[256];
	crc32::generate_table(table);
	unsigned crc_new = crc32::update(table, 0, p, MTU);
	printf("CRC vrfy %x %x\n", crc_old, crc_new);
	return crc_old == crc_new;
}
