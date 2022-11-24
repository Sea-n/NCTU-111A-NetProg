/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/sha.h>

#include <list>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
using namespace std;

#define WORKERS		2
#define MAX_ATTEMPTS 16
#define	errquit(m)	{ perror(m); exit(-1); }
#define	NIPQUAD(x)	((uint8_t*) &(x))[0], ((uint8_t*) &(x))[1], ((uint8_t*) &(x))[2], ((uint8_t*) &(x))[3]

static int jcount = 0;
static mutex mut;
static condition_variable cv;
static list<string> jobs;
static string status[2];

int update_status(long id, string pfx, string msg) {
	mut.lock();
	status[id] = "[" + pfx + "] " + msg;
	mut.unlock();
	return -1;
}

int do_job(long id, string server) {
	int i, rlen, port;
	size_t slash;
	struct sockaddr_in sin;
	struct hostent *ent;
	char buf[1024];
	static const char cmd[] = "GET /\r\n\r\n";

	slash = server.find('/');
	if(slash == string::npos || slash == 0 || slash+1 == server.size())
		return update_status(id, server, "Incorrect server/port format.");

	port = strtol(server.substr(slash+1).c_str(), NULL, 0);

	if((ent = gethostbyname2(server.substr(0, slash).c_str(), AF_INET)) == NULL)
		return update_status(id, server, "Resolve failed.");

	if(((*((uint32_t*) ent->h_addr_list[0])) & htonl(0xff000000)) == htonl(0x0a000000)
	|| ((*((uint32_t*) ent->h_addr_list[0])) & htonl(0xff000000)) == htonl(0x7f000000)
	|| ((*((uint32_t*) ent->h_addr_list[0])) & htonl(0xfff00000)) == htonl(0xac100000)
	|| ((*((uint32_t*) ent->h_addr_list[0])) & htonl(0xffff0000)) == htonl(0xc0a80000))
		return update_status(id, server, "Get from local or private address is not allowed.");

	if(*((uint32_t*) ent->h_addr_list[0]) == 0)
		return update_status(id, server, "Get from * is not allowed.");

	for(i = 0; i < 2; i++) {
		int s = -1;
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_port = htons(port);
		sin.sin_addr.s_addr = *((uint32_t*) ent->h_addr_list[0]);
		snprintf(buf, sizeof(buf), "Connecting to %u.%u.%u.%u:%u ... (%d)", NIPQUAD(sin.sin_addr.s_addr), ntohs(sin.sin_port), i+1);
		update_status(id, server, buf);

		if((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) goto try_again;
		if(connect(s, (struct sockaddr*) &sin, sizeof(sin)) < 0) goto try_again;
		if(write(s, cmd, sizeof(cmd)-1) < 0) goto try_again;
		if((rlen = read(s, buf, sizeof(buf)-1)) <= 0) goto try_again;

		/* got it! */
		buf[rlen] = '\0';
		for(i = 0; i < rlen; i++) {
			if(isspace(buf[i])) buf[i] = ' ';
			else if(!isprint(buf[i])) buf[i] = '.';
		}
		update_status(id, server, buf);
		close(s);
		break;
try_again:
		update_status(id, server, strerror(errno));
		if(s >= 0) close(s);
	}

	return 0;
}

void worker(long myid) {
	unique_lock<mutex> _lock(mut, defer_lock);
	while(1) {
		bool hasjob = false;
		string s = "";
		_lock.lock();
again:
		if(jobs.size() > 0) {
			s = jobs.front();
			jobs.pop_front();
			hasjob = true;
		}
		if(!hasjob) {
			cv.wait(_lock);
			goto again;
		}
		_lock.unlock();

		if(s == "") break;

		do_job(myid, s);
	}
}

void add_job(string s) {
	mut.lock();
	jobs.push_back(s);
	jcount++;
	if(jcount > MAX_ATTEMPTS) {
		printf("** Too many trials ...\n");
		_exit(-1);
	}
	mut.unlock();
	cv.notify_one();
}

void check_queue() {
	int i = 0;
	list<string>::iterator li;
	mut.lock();
	if(jobs.size() == 0) {
		printf("** No pending jobs.\n");
	} else {
		printf("==== Pending Jobs ====\n");
		for(i = 0, li = jobs.begin(); li != jobs.end(); i++, li++)
			printf("  #%-3d server = %s\n", i+1, (*li).c_str());
		printf("======================\n");
	}
	mut.unlock();
}

void view_worker() {
	printf("==== Worker Status ====\n");
	mut.lock();
	printf("Worker #1: %s\n", status[0] == "" ? "<idle>" : status[0].c_str());
	printf("Worker #2: %s\n", status[1] == "" ? "<idle>" : status[1].c_str());
	mut.unlock();
	printf("=======================\n");
}

char menu() {
	char buf[64];
	printf("\n"
		"-----------------------------\n"
		"      MENU                   \n"
		"-----------------------------\n"
		" r: request from a webserver\n"
		" c: check request queue\n"
		" v: view worker status\n"
		" q: quit\n"
		"-----------------------------\n"
		"> ");
	if(fgets(buf, sizeof(buf), stdin) == NULL) return 'q';
	return buf[0];
}

#include "base64.c"

int pow() {
	int i, len, olen, difficulty = 6;
	char buf[512], *token, *saveptr;
	unsigned char cksum[20];
	srand(time(0) ^ getpid());
	len = snprintf(buf, sizeof(buf), "%x", rand());
	printf(
		"Proof-of-Work Access Rate Control: Given a prefix P = '%s'.\n"
		"Please send me a base64-encoded string S so that the hexdigest of\n"
		"sha1(P+decode(S)) has at least %d leading zeros.\n"
		"Enter your string S: ",
		buf, difficulty);
	if(fgets(buf+len, sizeof(buf)-len, stdin) == NULL)
		return -1;
	if((token = strtok_r(buf, "\n\r", &saveptr)) == NULL)
		return -1;
	if(b64decode((unsigned char*) buf+len, strlen(buf+len), (unsigned char*) buf+len, &olen) == NULL)
		return -1;
	if(SHA1((unsigned char*) buf, len+olen, cksum) == NULL)
		return -1;
	for(i = 0; i < 20; i++) snprintf(&buf[i*2], 4, "%2.2x", cksum[i]);
	printf("SHA1: %s\n\n", buf);
	for(i = 0; i < difficulty; i++)
		if(buf[i] != '0') return -1;
	return 0;
}

int main() {
	int i;
	char c, buf[256], *ptr, *saveptr;

	setvbuf(stdin,  NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	if(pow() < 0) _exit(-1);

	for(i = 0; i < WORKERS; i++) (new thread(worker, i))->detach();

	printf("Welcome to the INP Web Checker Service!\n");
	while((c = menu()) != 'q') {
		switch(c) {
		case 'r':
			printf("Enter ``server-domain-name/port``: ");
			if(fgets(buf, sizeof(buf), stdin) != NULL
			&&(ptr = strtok_r(buf, " \t\n\r", &saveptr)) != NULL) {
				add_job(ptr);
				printf("** New request added: %s\n", ptr);
			} else {
				printf("** Bad user input!\n");
			}
			break;
		case 'c':
			check_queue();
			break;
		case 'v':
			view_worker();
			break;
		default:
			printf("** Unknown command. Please don't hack me! Q_Q\n");
		}
	}
	printf("\nThank you for using our service. Bye!\n");

	_exit(0);

	return 0;
}
