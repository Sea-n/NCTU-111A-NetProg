#include "mircd.h"

using namespace std;

User users[212057];

int main(int argc, char *argv[]) {
	struct sockaddr_in srvAddr, cliAddr;
	socklen_t addr_size = sizeof(cliAddr);
	int srvSock, n, one=1, cnt=0, k=0;
	time_t now; time(&now);
	char input[3072];
	struct timeval zero = {0, 0};
	fd_set fds;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		return 1;
	}
	signal(SIGPIPE, SIG_IGN);

	// Init TCP socket
	if ((srvSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "%s: (%d) Socket error\n", argv[0], errno);
		return 1;
	}
	setsockopt(srvSock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &one, sizeof(one));

	srvAddr.sin_family = AF_INET;
	srvAddr.sin_addr.s_addr = INADDR_ANY;
	srvAddr.sin_port = htons(atoi(argv[1]));

	if (bind(srvSock, (struct sockaddr*) &srvAddr, sizeof(srvAddr)) < 0) {
		fprintf(stderr, "%s: (%d) Bind error\n", argv[0], errno);
		return 1;
	}
	listen(srvSock, 42);

	for (;;) {
		FD_ZERO(&fds); FD_SET(srvSock, &fds);
		while (select(1024, &fds, NULL, NULL, &zero) > 0) {
			users[++k].sock = accept(srvSock, (struct sockaddr*) &cliAddr, &addr_size);
			if (users[k].sock < 0) {
				fprintf(stderr, "%s: (%d) Accept error\n", argv[0], errno);
				--k;
				usleep(100 * 1000);
				break;
			}
			cnt++;

			sprintf(users[k].addr, "%s:%d", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));
			strcpy(users[k].name, animals[(now * 401 + k * 263) % 877]);
			time(&users[k].created_at);

			senddf(k, "Welcome to the simple CHAT server\n");
			senddf(k, "Total %d users online now. Your name is <%s>\n", cnt, users[k].name);
			sendaf(k, "*** User <%s> has just landed on the server\n", users[k].name);
			printf("* client connected from %s\n", users[k].addr);
		}

		for (int uid=0; uid<212057; uid++) {
			if (!users[uid].created_at || users[uid].deleted_at)
				continue;
			FD_ZERO(&fds); FD_SET(users[uid].sock, &fds);
			if (select(1024, &fds, NULL, NULL, &zero) <= 0)
				continue;

			if ((n = read(users[uid].sock, input, 3070)) == 0) {
				sendaf(uid, "User <%s> has left the server\n", users[uid].name);
				printf("* client %s disconnected\n", users[uid].addr);
				time(&users[uid].deleted_at);
				close(users[uid].sock);
				cnt--;  // Current user count
				continue;
			}
			input[n-1] = '\0';  // newline -> eol

			if (input[0] != '/') {
				sendaf(uid, "<%s> %s\n", users[uid].name, input);
				continue;
			}

			// Commands
			if (strncmp(input, "/who", 4) == 0) {
				sendf(uid, "--------------------------------------------------\n");
				for (int i=0; i<212057; i++) {
					if (!users[i].created_at || users[i].deleted_at)
						continue;
					sendf(uid, "%c %-20s %s\n", i^uid?' ':'*', users[i].name, users[i].addr);
				}
				sendf(uid, "--------------------------------------------------\n");
			} else if (strncmp(input, "/name ", 6) == 0) {
				sendaf(uid, "*** User <%s> renamed to <%s>\n", users[uid].name, input+6);
				senddf(uid, "Nickname changed to <%s>\n", input+6);
				strcpy(users[uid].name, input+6);
			} else if (strncmp(input, "/top", 4) == 0) {
				senddf(uid, "Total   user: %d\n", k);
				senddf(uid, "Current user: %d\n", cnt);
				senddf(uid, "Send pointer: %d\n", users[uid].ptr);
			} else {
				senddf(uid, "Unknown or incomplete command <%s>\n", input);
			}
		}  // End for users
	}  // End main loop
}  // main function

int sendf(const int uid, const char *fmt, ...) {
	char buf[8192];
	va_list va;

	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);

	return send(users[uid].sock, buf, strlen(buf), 0);
}  // sendf

int senddf(const int uid, const char *fmt, ...) {
	struct tm *timeinfo;
	char buf[8192], buf2[8192];
	time_t now;
	va_list va;

	time(&now);
	timeinfo = localtime(&now);
	strftime(buf, 25, "%F %T *** ", timeinfo);
	strcpy(buf+24, fmt);

	va_start(va, fmt);
	vsprintf(buf2, buf, va);
	va_end(va);

	return send(users[uid].sock, buf2, strlen(buf2), 0);
}  // senddf

void sendaf(const int uid, const char *fmt, ...) {
	struct tm *timeinfo;
	char buf[4096];
	time_t now;
	va_list va;

	time(&now);
	timeinfo = localtime(&now);
	strftime(buf, 21, "%F %T ", timeinfo);

	va_start(va, fmt);
	vsprintf(buf+20, fmt, va);
	va_end(va);

	for (int i=0; i<212057; i++) {
		if (!users[i].created_at || users[i].deleted_at || i == uid)
			continue;
		send(users[i].sock, buf, strlen(buf), 0);
	}
}  // sendaf
