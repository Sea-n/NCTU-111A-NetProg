#include "server.h"

using namespace std;

int ptr, uid, cliSock;
User *users;
Msg *msg;

int main(int argc, char *argv[]) {
	struct sockaddr_in srvAddr, cliAddr;
	socklen_t addr_size = sizeof(cliAddr);
	int srvSock, pid, n, one=1, *cnt;
	int shm_user, shm_msg, shm_cnt;
	time_t now; time(&now);
	char input[3072];

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		return 1;
	}
	signal(SIGCHLD, proc_exit);

	// Shared Memory
	shm_user = shmget(IPC_PRIVATE, sizeof(User) * 212057, IPC_CREAT | 0666);
	shm_msg = shmget(IPC_PRIVATE, sizeof(Msg) * 2160, IPC_CREAT | 0666);
	shm_cnt = shmget(IPC_PRIVATE, sizeof(int) * 2, IPC_CREAT | 0666);
	users = (User*) shmat(shm_user, NULL, 0);
	msg = (Msg*) shmat(shm_msg, NULL, 0);
	cnt = (int*) shmat(shm_cnt, NULL, 0);
	cnt[0] = cnt[1] = 0;
	msg[0].uid = -1;  // Mark head

	// Clean old messages
	if ((pid = fork()) == 0) {
		for (ptr = 0; ; ) {
			while (msg[ptr].uid == 0)
				usleep(10 * 1000);
			msg[(ptr + 500) % 2160].uid = 0;
			ptr = (ptr+1) % 2160;
		}
	}

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

	// Accept new clients, gc is not implemented
	for (uid=1; uid<212057; uid++) {
		cliSock = accept(srvSock, (struct sockaddr*) &cliAddr, &addr_size);
		if (cliSock < 0) {
			fprintf(stderr, "%s: (%d) Accept error\n", argv[0], errno);
			exit(1);
		}
		cnt[0]++; cnt[1]++;

		if ((pid = fork()) == 0) {
			users = (User*) shmat(shm_user, NULL, 0);
			msg = (Msg*) shmat(shm_msg, NULL, 0);
			cnt = (int*) shmat(shm_cnt, NULL, 0);

			sprintf(users[uid].addr, "%s:%d", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));
			strcpy(users[uid].name, animals[(now * 401 + uid * 263) % 877]);
			time(&users[uid].created_at);
			close(srvSock);

			senddf("Welcome to the simple CHAT server\n");
			senddf("Total %d users online now. Your name is <%s>\n", cnt[1], users[uid].name);
			sendaf("*** User <%s> has just landed on the server\n", users[uid].name);
			printf("* client connected from %s\n", users[uid].addr);

			// Server message handler
			if ((pid = fork()) == 0) {
				ptr = 0;
				while (!msg[ptr].uid) ptr = (ptr+1) % 2160;
				while (msg[ptr].uid) ptr = (ptr+1) % 2160;

				while (!users[uid].deleted_at) {
					while (!msg[ptr].uid)
						usleep(10 * 1000);
					if (msg[ptr].uid != uid)
						sendf("%s", msg[ptr].text);
					ptr = (ptr+1) % 2160;
				}
				exit(0);
			}

			// Read user command or text
			while ((n = read(cliSock, input, 3070)) > 0) {
				input[n-1] = '\0';  // newline -> eol

				if (input[0] != '/') {
					sendaf("<%s> %s\n", users[uid].name, input);
					continue;
				}

				// Commands
				if (strncmp(input, "/who", 4) == 0) {
					sendf("--------------------------------------------------\n");
					for (int i=0; i<212057; i++) {
						if (!users[i].created_at || users[i].deleted_at)
							continue;
						sendf("%c %-20s %s\n", i^uid?' ':'*', users[i].name, users[i].addr);
					}
					sendf("--------------------------------------------------\n");
				} else if (strncmp(input, "/name ", 6) == 0) {
					sendaf("*** User <%s> renamed to <%s>\n", users[uid].name, input+6);
					senddf("Nickname changed to <%s>\n", input+6);
					strcpy(users[uid].name, input+6);
				} else if (strncmp(input, "/top", 4) == 0) {
					senddf("Total   user: %d\n", cnt[0]);
					senddf("Current user: %d\n", cnt[1]);
					senddf("Send pointer: %d\n", ptr);
				} else {
					senddf("Unknown or incomplete command <%s>\n", input);
				}
			}  // End while read

			sendaf("User <%s> has left the server\n", users[uid].name);
			printf("* client %s disconnected\n", users[uid].addr);
			time(&users[uid].deleted_at);
			cnt[1]--;  // Current user count
			exit(0);
		} else {  // fork process
			// If not closed, main process will reach 1024 fd limit
			close(cliSock);
		}
	}  // for uid
}  // main

void proc_exit(int sig) {
	if (sig == SIGCHLD)
		while (waitpid(-1, nullptr, WNOHANG) > 0) ;
}

int sendf(const char *fmt, ...) {
	char buf[8192];
	va_list va;

	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);

	return send(cliSock, buf, strlen(buf), 0);
}  // sendf

int senddf(const char *fmt, ...) {
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

	return send(cliSock, buf2, strlen(buf2), 0);
}  // senddf

void sendaf(const char *fmt, ...) {
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

	while (!msg[ptr].uid) ptr = (ptr+1) % 2160;
	while (msg[ptr].uid) ptr = (ptr+1) % 2160;
	msg[ptr].uid = uid;
	strcpy(msg[ptr].text, buf);
}  // sendaf
