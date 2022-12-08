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
#define CHUNK_BUF (CHUNK_PKT / 5 * 6)

using namespace std;

typedef struct {
	short index;
	unsigned char content[CHUNK_PKT] = {};
} RUP;  // Protocol

typedef struct {
	unsigned short chunks;
	unsigned short size[1000] = {};
	unsigned short padding[469] = {};
} RUF;  // Metadata
