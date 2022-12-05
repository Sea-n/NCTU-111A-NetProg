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
#define CHUNK (MTU - 2)

using namespace std;

typedef struct {
	short index;
	char content[CHUNK] = {};
} RUP;  // Protocol

typedef struct {
	short chunks;
	unsigned short size[1000] = {};
	char content[];
} RUF;  // File
