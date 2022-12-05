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
	int index;
	char content[CHUNK] = {};
} RUP;  // Protocol

typedef struct {
	int chunks;
	int size[1000] = {};
	char content[];
} RUF;  // File
