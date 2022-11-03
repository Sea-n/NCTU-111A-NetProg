#include "mircd.h"

using namespace std;

User users[MAX_USERS];
Channel chans[MAX_CHANS];

int main(int argc, char *argv[]) {
	struct sockaddr_in srvAddr, cliAddr;
	socklen_t addr_size = sizeof(cliAddr);
	int srvSock, uid, cid, n, i, one=1, cnt=0, muid=0;
	char lines[42][MAX_TEXT], cmd[MAX_TEXT], arg[MAX_TEXT], args[42][MAX_TEXT], buf[3072], *ptr;
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

	for (;;) {  // Main loop
		FD_ZERO(&fds); FD_SET(srvSock, &fds);
		while (select(1024, &fds, NULL, NULL, &zero) > 0) {
			users[++muid].sock = accept(srvSock, (struct sockaddr*) &cliAddr, &addr_size);
			if (users[muid].sock < 0) {
				fprintf(stderr, "%s: (%d) Accept error\n", argv[0], errno);
				--muid;
				usleep(100 * 1000);
				break;
			}
			cnt++;

			sprintf(users[muid].addr, "%s", inet_ntoa(cliAddr.sin_addr));
			time(&users[muid].created_at);

			printf("* client connected from %s\n", users[muid].addr);
		}

		for (uid=1; uid<MAX_USERS; uid++) {
			if (!users[uid].created_at || users[uid].deleted_at)
				continue;
			FD_ZERO(&fds); FD_SET(users[uid].sock, &fds);
			if (select(1024, &fds, NULL, NULL, &zero) <= 0)
				continue;

			if ((n = read(users[uid].sock, buf, 3070)) == 0) {
				printf("* client %s disconnected\n", users[uid].addr);
				time(&users[uid].deleted_at);
				close(users[uid].sock);
				cnt--;  // Current user count
				continue;
			}
			buf[n-1] = '\0';

			ptr = strtok(buf, "\r\n");
			for (i=0; ptr != nullptr; i++) {
				if (ptr[0])
					strcpy(lines[i], ptr);
				else
					i--;
				ptr = strtok(nullptr, "\r\n");
			}
			lines[i][0] = '\0';

			for (int ln=0; lines[ln][0]; ln++) {
				// Parse command
				strcpy(buf, lines[ln]);
				ptr = strtok(buf, " ");
				strcpy(cmd, ptr);
				ptr = strtok(nullptr, "");
				if (ptr == nullptr) {
					strcpy(arg, "");
					args[0][0] = '\0';
				} else {
					strcpy(arg, ptr);
					strcpy(buf, arg);
					for (i=0; ; i++) {
						if (buf[0] == ':') {
							strcpy(args[i], buf+1);
							args[i+1][0] = '\0';
							break;
						}
						ptr = strtok(buf, " ");
						strcpy(args[i], ptr);
						ptr = strtok(nullptr, "");
						if (ptr == nullptr) {
							args[i+1][0] = '\0';
							break;
						}
						strcpy(buf, ptr);
					}
				}  // Parse command

				if (strcmp(cmd, "PING") == 0) {
					sendf(uid, "PONG %s\n", arg);
				} else if (strcmp(cmd, "NICK") == 0) {
					strcpy(users[uid].name, args[0]);
				} else if (strcmp(cmd, "USER") == 0) {
					sendsf(uid, 0x1, ":Welcome to the minimized IRC daemon!");
					sendsf(uid, 251, ":There are %d users and 0 invisible on 1 server", cnt);
					sendsf(uid, 375, ":- mircd Message of the day -");
					sendsf(uid, 372, ":-  Hello, World!");
					sendsf(uid, 372, ":-               @                    _");
					sendsf(uid, 372, ":-   ____  ___   _   _ _   ____.     | |");
					sendsf(uid, 372, ":-  /  _ `'_  \\ | | | '_/ /  __|  ___| |");
					sendsf(uid, 372, ":-  | | | | | | | | | |   | |    /  _  |");
					sendsf(uid, 372, ":-  | | | | | | | | | |   | |__  | |_| |");
					sendsf(uid, 372, ":-  |_| |_| |_| |_| |_|   \\____| \\___,_|");
					sendsf(uid, 372, ":-  minimized internet relay chat daemon");
					sendsf(uid, 372, ":-");
					sendsf(uid, 376, ":End of message of the day");
				} else if (strcmp(cmd, "JOIN") == 0) {
					if (args[0][0] == '\0') {
						sendsf(uid, 461, "JOIN :Not enought parameters");
						continue;;
					}
					strcpy(buf+1, args[0] + (args[0][0] == '#' ? 1 : 0));
					buf[0] = '#';  // Prepend #hash if not exist

					for (cid=1; cid<MAX_CHANS; cid++)
						if (strcmp(buf, chans[cid].name) == 0)
							break;  // Channel found
					if (cid == MAX_CHANS) {  // Channel not found
						for (cid=1; cid<MAX_CHANS; cid++)
							if (!chans[cid].created_at)
								break;  // Empty slot
						time(&chans[cid].created_at);
						strcpy(chans[cid].name, buf);
					}
					for (i=MAX_USERS-1; i>0; i--)  // Append user list
						if (chans[cid].users[i-1] && !chans[cid].users[i]) {
							chans[cid].users[i] = uid;
							chans[cid].cnt++;
							break;
						}

					sendf(uid, ":%s JOIN %s\n", users[uid].name, chans[cid].name);
					if (chans[cid].topic[0] == '\0')
						sendsf(uid, 331, "%s :No topic is set", chans[cid].name);
					else
						sendsf(uid, 332, "%s :%s", chans[cid].name, chans[cid].topic);

					// List channel users
					sendf(uid, ":mircd 353 %s %s :", users[uid].name, chans[cid].name);
					for (i=1; i<MAX_USERS; i++) {
						if (!chans[cid].users[i])
							continue;
						sendf(uid, "%s ", users[i].name);
					}
					sendf(uid, "\n");
					sendsf(uid, 366, "%s :End of Names List", chans[cid].name);
				} else if (strcmp(cmd, "TOPIC") == 0) {
					if (args[0][0] == '\0') {
						sendsf(uid, 461, "TOPIC :Not enought parameters");
						continue;
					}
					strcpy(buf+1, args[0] + (args[0][0] == '#' ? 1 : 0));
					buf[0] = '#';  // Prepend #hash if not exist

					for (cid=1; cid<MAX_CHANS; cid++)
						if (strcmp(buf, chans[cid].name) == 0)
							break;  // Channel found
					if (cid == MAX_CHANS) {
						sendsf(uid, 442, "%s :You are not on that channel", buf);
						continue;
					}

					for (i=1; i<MAX_USERS; i++)
						if (chans[cid].users[i] == uid)
							break;  // User found
					if (i == MAX_USERS) {
						sendsf(uid, 442, "%s :You are not on that channel", buf);
						continue;
					}

					if (args[1][0])  // Set topic
						strcpy(chans[cid].topic, args[1]);
					if (chans[cid].topic[0] == '\0')  // Show topic
						sendsf(uid, 331, "%s :No topic is set", chans[cid].name);
					else
						sendsf(uid, 332, "%s :%s", chans[cid].name, chans[cid].topic);
				} else if (strcmp(cmd, "PRIVMSG") == 0) {
					if (args[0][0] == '\0') {
						sendsf(uid, 411, ":No recipient given (PRIVMSG)");
						continue;
					}
					if (args[1][0] == '\0') {
						sendsf(uid, 412, ":No text to send");
						continue;
					}

					for (cid=1; cid<MAX_CHANS; cid++)
						if (strcmp(args[0], chans[cid].name) == 0)
							break;  // Channel found
					if (cid == MAX_CHANS) {
						sendsf(uid, 401, "%s :No such nick/channel", args[0]);
						continue;
					}
					for (i=1; i<MAX_USERS; i++)
						if (chans[cid].users[i] && i != uid)
							sendf(i, ":%s PRIVMSG %s :%s\n", users[uid].name, chans[cid].name, args[1]);
				} else if (strcmp(cmd, "PART") == 0) {
					if (args[0][0] == '\0') {
						sendsf(uid, 461, "PART :Not enought parameters");
						continue;
					}

					for (cid=1; cid<MAX_CHANS; cid++)
						if (strcmp(args[0], chans[cid].name) == 0)
							break;  // Channel found
					if (cid == MAX_CHANS) {
						sendsf(uid, 403, "%s :No such channel", args[0]);
						continue;
					}
					for (i=1; i<MAX_USERS; i++)
						if (chans[cid].users[i] == uid)
							break;  // User found
					if (i == MAX_USERS) {
						sendsf(uid, 442, "%s :You are not on that channel", buf);
						continue;
					}
					chans[cid].users[i] = 0;
					chans[cid].cnt--;
					sendf(uid, ":%s PART :%s\n", users[uid].name, chans[cid].name);
				} else if (strcmp(cmd, "USERS") == 0) {
					sendsf(uid, 392, ":%-32s Terminal  Host", "UserID");
					for (i=0; i<MAX_USERS; i++) {
						if (!users[i].created_at || users[i].deleted_at)
							continue;
						sendsf(uid, 393, ":%-32s %-9s %s", users[i].name, "-", users[i].addr);
					}
					sendsf(uid, 394, ":End of users");
				} else if (strcmp(cmd, "LIST") == 0) {
					sendsf(uid, 321, "Channel :Users Name");
					for (i=0; i<MAX_CHANS; i++) {
						if (!chans[i].created_at || chans[i].deleted_at)
							continue;
						if (args[0][0] && strcmp(args[0], chans[i].name) != 0)
							continue;
						sendsf(uid, 322, "%s %d :%s", chans[i].name, chans[i].cnt, chans[i].topic);
					}
					sendsf(uid, 323, ":End of List");
				} else if (strcmp(cmd, "QUIT") == 0) {
					// pass
				} else {
					sendsf(uid, 421, "%s :Unknown command", cmd);
				}
			}  // One line of command
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

int sendsf(const int uid, const int code, const char *fmt, ...) {
	char new_fmt[8192], buf[8192];
	va_list va;

	sprintf(new_fmt, ":mircd %03d %s %s\n", code, users[uid].name, fmt);

	va_start(va, new_fmt);
	vsprintf(buf, new_fmt, va);
	va_end(va);

	return send(users[uid].sock, buf, strlen(buf), 0);
}  // sendsf
