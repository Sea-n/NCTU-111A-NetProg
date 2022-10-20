#include <arpa/inet.h>
#include <sys/wait.h>
#include <iostream>
#include <unistd.h>

using namespace std;

void proc_exit(int sig) {
	if (sig == SIGCHLD) wait(nullptr);
}

int main(int argc, char *argv[]) {
	struct sockaddr_in srvAddr, cliAddr;
	int srvSock, cliSock, pid, one=1;
	socklen_t addr_size;

	if (argc < 3) {
		cerr << "Usage: " << argv[0] << " <port> <command> [argument]\n";
		return 1;
	}
	signal(SIGCHLD, proc_exit);

	// Init TCP socket
	if ((srvSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		cerr << "Socket error " << srvSock << ".\n";
		return 1;
	}

	setsockopt(srvSock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &one, sizeof(one));

	srvAddr.sin_family = AF_INET;
	srvAddr.sin_addr.s_addr = INADDR_ANY;
	srvAddr.sin_port = htons(atoi(argv[1]));

	if (bind(srvSock, (struct sockaddr*) &srvAddr, sizeof(srvAddr)) < 0) {
		cerr << "Bind error.\n";
		return 1;
	}
	listen(srvSock, 42);

	for (;;) {
		cliSock = accept(srvSock, (struct sockaddr*) &cliAddr, &addr_size);
		if (cliSock < 0) {
			cerr << "Accept error " << cliSock << ".\n";
			continue;
		}

		printf("Connection accepted from %s:%d\n", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));

		if ((pid = fork()) == 0) {
			close(srvSock);
			dup2(cliSock, 0);
			dup2(cliSock, 1);
			dup2(cliSock, 2);
			execvp(argv[2], argv+2);

			cerr << argv[0] << ": Command '" << argv[2] << "' not found.\n";
			exit(1);
		}
	}
}
