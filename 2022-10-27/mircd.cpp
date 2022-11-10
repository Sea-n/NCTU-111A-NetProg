#include "mircd.h"

using namespace std;

User users[MAX_USERS];
Channel chans[MAX_CHANS];
int user_cnt = 0;

int main(int argc, char *argv[]) {
	struct sockaddr_in srvAddr, cliAddr;
	socklen_t addr_size = sizeof(cliAddr);
	int srvSock, uid, n, i, one=1, muid=0;
	char lines[42][MAX_TEXT], buf[MAX_TEXT*42], *ptr;
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
		while (select(1024, &fds, NULL, NULL, &zero) > 0) {  // New connections
			users[++muid].sock = accept(srvSock, (struct sockaddr*) &cliAddr, &addr_size);
			if (users[muid].sock < 0) {
				fprintf(stderr, "%s: (%d) Accept error\n", argv[0], errno);
				--muid;
				usleep(100 * 1000);
				break;
			}
			user_cnt++;

			sprintf(users[muid].addr, "%s", inet_ntoa(cliAddr.sin_addr));
			time(&users[muid].created_at);

			printf("* client connected from %s\n", users[muid].addr);
		}

		for (uid=1; uid<MAX_USERS; uid++) {  // Existing users
			if (!users[uid].created_at || users[uid].deleted_at)
				continue;
			FD_ZERO(&fds); FD_SET(users[uid].sock, &fds);
			if (select(1024, &fds, NULL, NULL, &zero) <= 0)
				continue;

			if ((n = read(users[uid].sock, buf, MAX_TEXT*40)) == 0) {
				printf("* client %s disconnected\n", users[uid].addr);
				user_quit(uid);
				continue;
			}
			buf[n-1] = '\0';

			ptr = strtok(buf, "\r\n");
			for (i=0; ptr != nullptr; ptr = strtok(nullptr, "\r\n"))
				if (ptr[0])
					strcpy(lines[i++], ptr);
			lines[i][0] = '\0';

			for (i=0; lines[i][0]; i++)
				proc_cmd(uid, lines[i]);
		}
	}  // End main loop
}  // main function

void proc_cmd(const int uid, const char *line) {
	char cmd[MAX_TEXT], args[42][MAX_TEXT], buf[MAX_TEXT], *ptr;
	int cid, i;

	printf("proc_cmd(%d, %s)\n", uid, line);
	strcpy(buf, line);
	ptr = strtok(buf, " ");
	strcpy(cmd, ptr);
	ptr = strtok(nullptr, "");
	if (ptr == nullptr) {
		args[0][0] = '\0';
	} else {
		strcpy(buf, ptr);
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
		sendf(uid, "PONG %s\n", line+5);
	} else if (strcmp(cmd, "NICK") == 0) {
		strcpy(users[uid].name, args[0]);
	} else if (strcmp(cmd, "USER") == 0) {
		sendsf(uid, 0x1, ":Welcome to the minimized IRC daemon!");
		sendsf(uid, 251, ":There are %d users and 0 invisible on 1 server", user_cnt);
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
		CHECK_PARAM();

		cid = cid_by_name(args[0]);
		if (!cid) {  // Channel not found
			while (chans[++cid].created_at) ;
			time(&chans[cid].created_at);
			chans[cid].name[0] = '#';
			strcpy(chans[cid].name+1, args[0] + (args[0][0] == '#' ? 1 : 0));
		}
		for (i=MAX_USERS-1; i>0; i--)  // Append user list
			if (chans[cid].users[i-1] && !chans[cid].users[i]) {
				chans[cid].users[i] = uid;
				chans[cid].user_cnt++;
				break;
			}

		sendf(uid, ":%s JOIN %s\n", users[uid].name, chans[cid].name);
		if (chans[cid].topic[0] == '\0')
			sendsf(uid, 331, "%s :No topic is set", chans[cid].name);
		else
			sendsf(uid, 332, "%s :%s", chans[cid].name, chans[cid].topic);

		rpl_namereply(uid, cid);
	} else if (strcmp(cmd, "TOPIC") == 0) {
		CHECK_PARAM();

		if (!(cid = cid_by_name(args[0])) || !uid_in_cid(uid, cid)) {
			sendsf(uid, 442, "%s :You are not on that channel", args[0]);
			return;
		}

		if (args[1][0])  // Set topic
			strcpy(chans[cid].topic, args[1]);
		rpl_topic(uid, cid);
	} else if (strcmp(cmd, "NAMES") == 0) {
		if (!args[0][0])
			for (cid=1; cid<MAX_CHANS; cid++)
				rpl_namereply(uid, cid);
		else if ((cid = cid_by_name(args[0])))
			rpl_namereply(uid, cid);
		else  // Specified non-existing channel
			sendsf(uid, 366, "%s :End of Names List", args[0]);
	} else if (strcmp(cmd, "PRIVMSG") == 0) {
		if (args[0][0] == '\0') {
			sendsf(uid, 411, ":No recipient given (PRIVMSG)");
			return;
		}
		if (args[1][0] == '\0') {
			sendsf(uid, 412, ":No text to send");
			return;
		}

		if (!(cid = cid_by_name(args[0]))) {
			sendsf(uid, 401, "%s :No such nick/channel", args[0]);
			return;
		}
		for (i=1; i<MAX_USERS; i++)
			if (chans[cid].users[i] && i != uid)
				sendf(i, ":%s PRIVMSG %s :%s\n", users[uid].name, chans[cid].name, args[1]);
	} else if (strcmp(cmd, "PART") == 0) {
		CHECK_PARAM();

		if (!(cid = cid_by_name(args[0]))) {
			sendsf(uid, 403, "%s :No such channel", args[0]);
			return;
		}
		if ((i = uid_in_cid(uid, cid))) {
			sendsf(uid, 442, "%s :You are not on that channel", args[0]);
			return;
		}
		chans[cid].users[i] = 0;
		chans[cid].user_cnt--;
		sendf(uid, ":%s PART :%s\n", users[uid].name, chans[cid].name);
	} else if (strcmp(cmd, "USERS") == 0) {
		sendsf(uid, 392, ":%-32s Terminal  Host", "UserID");
		for (i=0; i<MAX_USERS; i++)
			if (users[i].created_at && !users[i].deleted_at)
				sendsf(uid, 393, ":%-32s %-9s %s", users[i].name, "-", users[i].addr);
		sendsf(uid, 394, ":End of users");
	} else if (strcmp(cmd, "LIST") == 0) {
		sendsf(uid, 321, "Channel :Users Name");
		for (cid=0; cid<MAX_CHANS; cid++) {
			if (!chans[cid].created_at || chans[cid].deleted_at)
				continue;
			if (args[0][0] && strcmp(args[0], chans[cid].name) != 0)
				continue;
			sendsf(uid, 322, "%s %d :%s", chans[cid].name, chans[cid].user_cnt, chans[cid].topic);
		}
		sendsf(uid, 323, ":End of List");
	} else if (strcmp(cmd, "QUIT") == 0) {
		printf("* client %s quitted\n", users[uid].addr);
		user_quit(uid);
	} else {
		sendsf(uid, 421, "%s :Unknown command", cmd);
	}
}

int sendf(const int uid, const char *fmt, ...) {
	char buf[MAX_TEXT*42];
	va_list va;

	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);

	printf("sendf(%d): %s\n", uid, buf);
	return send(users[uid].sock, buf, strlen(buf), 0);
}  // sendf

int sendsf(const int uid, const int code, const char *fmt, ...) {
	char new_fmt[MAX_TEXT*42], buf[MAX_TEXT*42];
	va_list va;

	sprintf(new_fmt, ":mircd %03d %s %s\n", code, users[uid].name, fmt);

	va_start(va, fmt);
	vsprintf(buf, new_fmt, va);
	va_end(va);

	printf("sendsf(%d): %s", uid, buf);
	return send(users[uid].sock, buf, strlen(buf), 0);
}  // sendsf

int cid_by_name(const char *name) {
	char new_name[MAX_TEXT] = "#";
	strcpy(new_name+1, name + (name[0] == '#' ? 1 : 0));
	for (int cid=1; cid<MAX_CHANS; cid++)
		if (strcmp(new_name, chans[cid].name) == 0)
			return cid;
	return 0;
}

int uid_in_cid(int uid, int cid) {
	for (int i=1; i<MAX_USERS; i++)
		if (chans[cid].users[i] == uid)
			return i;
	return 0;
}

void user_quit(int uid) {
	time(&users[uid].deleted_at);
	close(users[uid].sock);
	user_cnt--;

	for (int cid=1; cid<MAX_CHANS; cid++)
		for (int i=1; i<MAX_USERS; i++)
			if (chans[cid].users[i] == uid) {
				chans[cid].users[i] = 0;
				chans[cid].user_cnt--;
			}
}
void rpl_namereply(int uid, int cid) {
	sendf(uid, ":mircd 353 %s %s :", users[uid].name, chans[cid].name);
	for (int i=1; i<MAX_USERS; i++) {
		if (!chans[cid].users[i])
			continue;
		sendf(uid, "%s ", users[i].name);
	}
	sendf(uid, "\n");
	sendsf(uid, 366, "%s :End of Names List", chans[cid].name);
}

void rpl_topic(int uid, int cid) {
	if (chans[cid].topic[0] == '\0')
		sendsf(uid, 331, "%s :No topic is set", chans[cid].name);
	else
		sendsf(uid, 332, "%s :%s", chans[cid].name, chans[cid].topic);
}
