#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#define err_quit(m) { perror(m); exit(-1); }

#define NIPQUAD(s)	((unsigned char *) &s)[0], ((unsigned char *) &s)[1], \
					((unsigned char *) &s)[2], ((unsigned char *) &s)[3]

typedef struct {
	unsigned seq;
	struct timeval tv;
}	ping_t;

double tv2ms(struct timeval *tv);
void do_send(int sig);
