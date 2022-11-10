#include "server.h"

using namespace std;

int main(int argc, char *argv[]) {
	int cmdSock, datSock, cliSock, n, one=1, cnt=0;
	struct sockaddr_in cmdAddr, datAddr, cliAddr;
	socklen_t addr_size = sizeof(cliAddr);
	vector<int> cmdSocks, datSocks;
	struct timeval zero = {0, 0};
	char buf[MAX_TEXT];
	double elap, mbps;
	timeval rst, now;
	fd_set fds;

	// Init TCP socket
	signal(SIGPIPE, SIG_IGN);
	cmdSock = socket(AF_INET, SOCK_STREAM, 0);
	datSock = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(cmdSock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &one, sizeof(one));
	setsockopt(datSock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &one, sizeof(one));

	cmdAddr.sin_family = AF_INET;
	datAddr.sin_family = AF_INET;
	cmdAddr.sin_addr.s_addr = INADDR_ANY;
	datAddr.sin_addr.s_addr = INADDR_ANY;
	cmdAddr.sin_port = htons(CMD_PORT);
	datAddr.sin_port = htons(DAT_PORT);

	bind(cmdSock, (struct sockaddr*) &cmdAddr, sizeof(cmdAddr));
	bind(datSock, (struct sockaddr*) &datAddr, sizeof(datAddr));
	listen(cmdSock, 42);
	listen(datSock, 42);

	gettimeofday(&rst, NULL);

	for (;;) {  // Main loop
		FD_ZERO(&fds); FD_SET(cmdSock, &fds);  // New connections for command
		while (select(1024, &fds, NULL, NULL, &zero) > 0) {
			cliSock = accept(cmdSock, (struct sockaddr*) &cliAddr, &addr_size);
			if (cliSock < 0) {
				fprintf(stderr, "%s: (%d) Accept error\n", argv[0], errno);
				usleep(100 * 1000);
				break;
			}
			cmdSocks.push_back(cliSock);

			printf("cmd connected from %s:%d\n", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));
		}

		FD_ZERO(&fds); FD_SET(datSock, &fds);  // New connections for data
		while (select(1024, &fds, NULL, NULL, &zero) > 0) {
			cliSock = accept(datSock, (struct sockaddr*) &cliAddr, &addr_size);
			if (cliSock < 0) {
				fprintf(stderr, "%s: (%d) Accept error\n", argv[0], errno);
				usleep(100 * 1000);
				break;
			}
			datSocks.push_back(cliSock);

			printf("dat connected from %s:%d\n", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));
		}

		for (int sock : cmdSocks) {
			FD_ZERO(&fds); FD_SET(sock, &fds);
			if (select(1024, &fds, NULL, NULL, &zero) <= 0)
				continue;

			if ((n = read(sock, buf, MAX_TEXT)) == 0) {
				remove(cmdSocks.begin(), cmdSocks.end(), sock);
				continue;
			}
			buf[n-1] = '\0';

			for (int i=1; i<n-1; i++)
				buf[i] |= 0x20;  // To lower case

			printf("recv(%d): %s\n", sock, buf);

			if (strcmp(buf, "/ping") == 0) {
				sendf(sock, "PONG");
			} else if (strcmp(buf, "/clients") == 0) {
				sendf(sock, "CLIENTS %d", datSocks.size());
			} else if (strcmp(buf, "/report") == 0) {
				gettimeofday(&now, NULL);
				elap = (now.tv_sec - rst.tv_sec) + 1e-6 * (now.tv_usec - rst.tv_usec);
				mbps = 8 * cnt / 1000. / 1000. / elap;
				sendf(sock, "REPORT %d %ld %.6lfs %.6lfMbps", cnt, elap, mbps);
			} else if (strcmp(buf, "/reset") == 0) {
				sendf(sock, "RESET %d", cnt);
				gettimeofday(&rst, NULL);
				cnt = 0;
			} else {
				sendf(sock, "UNKNOWN COMMAND '%s'", buf);
			}
		}

		for (int sock : datSocks) {
			FD_ZERO(&fds); FD_SET(sock, &fds);
			if (select(1024, &fds, NULL, NULL, &zero) <= 0)
				continue;
			n = read(sock, buf, MAX_TEXT);
			if (n == 0)
				remove(datSocks.begin(), datSocks.end(), sock);
			cnt += n;
		}
	}  // End main loop
}  // main function

int sendf(const int sock, const char *fmt, ...) {
	char new_fmt[MAX_TEXT], buf[MAX_TEXT];
	timeval now;
	va_list va;

	gettimeofday(&now, NULL);
	sprintf(new_fmt, "%ld.%06ld %s\n", now.tv_sec, now.tv_usec, fmt);

	va_start(va, fmt);
	vsprintf(buf, new_fmt, va);
	va_end(va);

	printf("sendf(%d): %s", sock, buf);
	return send(sock, buf, strlen(buf), 0);
}  // sendf
