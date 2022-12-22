#include <arpa/inet.h>
#include <linux/ip.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <cstdio>

#define MTU 1500
#define CHUNK (MTU - 22)

using namespace std;

typedef struct {
	int16_t index;
	unsigned char content[CHUNK] = {};
} RUP;  // Protocol

typedef struct {
	uint16_t chunks;
	uint16_t size[1001] = {};
} RUM;  // Metadata
