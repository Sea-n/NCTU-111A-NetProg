#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <cstdio>
#include <string>
#include <vector>

#define MTU (1500 - 28)
#define F first
#define S second

using namespace std;

typedef struct {
	int id : 16;
	int qr : 1;
	int opcode : 4;
	int aa : 1;
	int tc : 1;
	int rd : 1;
	int ra : 1;
	int z : 3;
	int rcode : 4;
	int qdcount_high : 8;
	int qdcount : 8;
	int ancount_high : 8;
	int ancount : 8;
	int nscount_high : 8;
	int nscount : 8;
	int arcount_high : 8;
	int arcount : 8;
} DNS;

typedef struct {
	string name;
	int ttl, clazz, type, rdlen;
	char rdata[512];
} RR;

typedef pair<string, vector<RR>> ZONE;
