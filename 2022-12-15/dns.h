#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <algorithm>
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

enum RR_TYPE : int {
	TYPE_A = 1,
	TYPE_NS = 2,
	TYPE_CNAME = 5,
	TYPE_SOA = 6,
	TYPE_MX = 15,
	TYPE_TXT = 16,
	TYPE_AAAA = 28,
	TYPE_UNKNOWN = -1
};

typedef struct {
	int id : 16;

	int rd : 1;
	int tc : 1;
	int aa : 1;
	int opcode : 4;
	int qr : 1;

	int rcode : 4;
	int z : 3;
	int ra : 1;

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
	string name, fqdn, full_name, need_a_rr;
	int ttl, clazz, type, rdlen, buf_len;
	char rdata[512];
	char buf[1024];
} RR;

typedef pair<string, vector<RR>> ZONE;

string fqdn2name(const char *fqdn);
string name2fqdn(const char *name);
