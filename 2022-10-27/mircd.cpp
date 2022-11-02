#include "mircd.h"

using namespace std;

User users[MAX_USERS];
Channel chans[MAX_CHANS];

int main(int argc, char *argv[]) {
	struct sockaddr_in srvAddr, cliAddr;
	socklen_t addr_size = sizeof(cliAddr);
	int srvSock, uid, cid, n, one=1, cnt=0, k=0;
	char *cmd, *arg, args[42][MAX_TEXT], buf[3072], space[2]=" ";
	time_t now; time(&now);
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

			sprintf(users[k].addr, "%s", inet_ntoa(cliAddr.sin_addr));
			strcpy(users[k].name, animals[(now * 401 + k * 263) % 877]);
			time(&users[k].created_at);

			printf("* client connected from %s\n", users[k].addr);
		}

		for (uid=1; uid<MAX_USERS; uid++) {
			if (!users[uid].created_at || users[uid].deleted_at)
				continue;
			FD_ZERO(&fds); FD_SET(users[uid].sock, &fds);
			if (select(1024, &fds, NULL, NULL, &zero) <= 0)
				continue;

			if ((n = read(users[uid].sock, buf, 3070)) == 0) {
				sendaf(uid, "User <%s> has left the server\n", users[uid].name);
				printf("* client %s disconnected\n", users[uid].addr);
				time(&users[uid].deleted_at);
				close(users[uid].sock);
				cnt--;  // Current user count
				continue;
			}
			buf[n-1] = '\0';  // newline -> eol

			cmd = strtok(buf, space);
			arg = strtok(nullptr, space);
			// TODO: arg -> args
			strcpy(args[0], arg);
			if (strcmp(cmd, "NICK") == 0) {
				// TODO: Check USER command
				strcpy(users[uid].name, arg);
				sendaf(uid, "*** User <%s> joined the server\n", users[uid].name);
				sendf(uid, ":mircd 001 %s :Welcome to the minimized IRC daemon!\n", users[uid].name);
				sendf(uid, ":mircd 251 %s :There are %d users and 0 invisible on 1 server\n", users[uid].name, cnt);
				sendf(uid, ":mircd 375 %s :- mircd Message of the day -\n", users[uid].name);
				sendf(uid, ":mircd 372 %s :-  Hello, World!\n", users[uid].name);
				sendf(uid, ":mircd 372 %s :-               @                    _\n", users[uid].name);
				sendf(uid, ":mircd 372 %s :-   ____  ___   _   _ _   ____.     | |\n", users[uid].name);
				sendf(uid, ":mircd 372 %s :-  /  _ `'_  \\ | | | '_/ /  __|  ___| |\n", users[uid].name);
				sendf(uid, ":mircd 372 %s :-  | | | | | | | | | |   | |    /  _  |\n", users[uid].name);
				sendf(uid, ":mircd 372 %s :-  | | | | | | | | | |   | |__  | |_| |\n", users[uid].name);
				sendf(uid, ":mircd 372 %s :-  |_| |_| |_| |_| |_|   \\____| \\___,_|\n", users[uid].name);
				sendf(uid, ":mircd 372 %s :-  minimized internet relay chat daemon\n", users[uid].name);
				sendf(uid, ":mircd 372 %s :-\n", users[uid].name);
				sendf(uid, ":mircd 376 %s :End of message of the day\n", users[uid].name);
			} else if (strcmp(cmd, "LIST") == 0) {
				// TODO: Specify channel
				sendf(uid, ":mircd 321 %s Channel :Users Name\n", users[uid].name);
				for (int i=0; i<MAX_CHANS; i++) {
					if (!chans[i].created_at || chans[i].deleted_at)
						continue;
					sendf(uid, ":mircd 322 %s %s %s :%s\n", users[uid].name, chans[i].name, chans[i].cnt, chans[i].topic);
				}
				sendf(uid, ":mircd 323 %s :End of List\n", users[uid].name);
			} else if (strcmp(cmd, "JOIN") == 0) {
				for (cid=1; cid<MAX_CHANS; cid++)
					if (strcmp(arg, chans[cid].name) == 0)
						break;
				if (cid == MAX_CHANS) {  // Channel not found
					for (cid=1; cid<MAX_CHANS; cid++)
						if (!chans[cid].created_at)
							break;
					time(&chans[cid].created_at);
					strcpy(chans[cid].name, arg);
				}
				for (int i=MAX_USERS-1; i>1; i--)  // Append user list
					if (chans[cid].users[i-1] && !chans[cid].users[i]) {
						chans[cid].users[i] = uid;
						break;
					}

				sendf(uid, ":%s JOIN %s\n", users[uid].name, chans[cid].name);
				if (chans[cid].topic[0] == '\0')
					sendf(uid, ":mircd 331 %s %s :No topic is set\n", users[uid].name, chans[cid].name);
				else
					sendf(uid, ":mircd 332 %s %s :%s\n", users[uid].name, chans[cid].name, chans[cid].topic);

				// List channel users
				sendf(uid, ":mircd 353 %s %s :", users[uid].name, chans[cid].name);
				for (int i=1; i<MAX_USERS; i++) {
					if (!chans[cid].users[i])
						continue;
					sendf(uid, "%s ", users[i].name);
				}
				sendf(uid, "\n");
				sendf(uid, ":mircd 366 %s %s :End of Names List\n", users[uid].name, chans[cid].name);
			} else if (strcmp(cmd, "TOPIC") == 0) {
				// TODO
			} else if (strcmp(cmd, "PING") == 0) {
				sendf(uid, "PONG %s\n", arg);
			} else {
				sendf(uid, ":mircd 421 %s %s :Unknown command\n", users[uid].name, cmd);
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

	for (int i=1; i<MAX_USERS; i++) {
		if (!users[i].created_at || users[i].deleted_at || i == uid)
			continue;
		send(users[i].sock, buf, strlen(buf), 0);
	}
}  // sendaf
