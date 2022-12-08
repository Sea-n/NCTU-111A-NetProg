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
#define CHUNK_PKT (MTU - 2)
#define CHUNK_BUF (CHUNK_PKT / 14 * 17)

using namespace std;

typedef struct {
	int16_t index;
	unsigned char content[CHUNK_PKT] = {};
} RUP;  // Protocol

typedef struct {
	uint16_t chunks;
	uint16_t size[1001] = {};
	uint16_t padding[468] = {};
} RUM;  // Metadata
