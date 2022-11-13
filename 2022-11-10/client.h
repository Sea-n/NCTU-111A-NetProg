#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>

#define MAX_TEXT 890604

void term(int sig);
