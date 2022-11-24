/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include <condition_variable>
#include <openssl/sha.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <string>
#include <thread>
#include <mutex>
#include <list>
#include "base64.cpp"

#define WORKERS		2
#define MAX_ATTEMPTS 16
#define	errquit(m)	{ perror(m); exit(-1); }
#define	NIPQUAD(x)	((uint8_t*) &(x))[0], ((uint8_t*) &(x))[1], ((uint8_t*) &(x))[2], ((uint8_t*) &(x))[3]

using namespace std;

int update_status(long id, string pfx, string msg);
int do_job(long id, string server);
void worker(long myid);
void add_job(string s);
void check_queue();
void view_worker();
char menu();
int pow();
