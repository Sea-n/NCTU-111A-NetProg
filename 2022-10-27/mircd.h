#include <arpa/inet.h>
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <cstdarg>
#include <cstring>

#define MAX_USERS 18185
#define MAX_CHANS 9916
#define MAX_TEXT 1024

int sendf(const int uid, const char *fmt, ...);
int sendsf(const int uid, const int code, const char *fmt, ...);
int cid_by_name(const char *name);
bool uid_in_cid(int uid, int cid);
void rpl_namereply(int uid, int cid);
void rpl_topic(int uid, int cid);

struct User {
	time_t created_at, deleted_at;
	char name[MAX_TEXT], addr[64];
	int sock;
};

struct Channel {
	time_t created_at, deleted_at;
	char name[MAX_TEXT], topic[MAX_TEXT];
	int cnt=0, users[MAX_USERS] = {-1};
};
