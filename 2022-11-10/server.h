#include <arpa/inet.h>
#include <sys/time.h>
#include <algorithm>
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <cstdarg>
#include <cstring>
#include <vector>

#define CMD_PORT 9998
#define DAT_PORT 9999
#define MAX_TEXT 890604

#define ALL(x) (x).begin(), (x).end()

int sendf(const int sock, const char *fmt, ...);
