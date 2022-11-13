#include "client.h"

using namespace std;

bool is_running = true;

int main(int argc, char *argv[]) {
	struct sockaddr_in cmdAddr, datAddr, cliAddr;
	char send_buf[MAX_TEXT], recv_buf[MAX_TEXT];
	socklen_t addr_size = sizeof(cliAddr);
	struct hostent *hostname;
	int cmdSock, datSocks[42], n;

	if (argc < 3) {
		cerr << "Usage: " << argv[0] << " <hostname> <port>\n";
		return 1;
	}
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, term);
	signal(SIGINT, term);

	// Init TCP socket
	hostname = gethostbyname(argv[1]);
	cmdAddr.sin_family = AF_INET;
	datAddr.sin_family = AF_INET;
	cmdAddr.sin_addr.s_addr = *((unsigned long *) hostname->h_addr);
	datAddr.sin_addr.s_addr = *((unsigned long *) hostname->h_addr);
	cmdAddr.sin_port = htons(atoi(argv[2]));
	datAddr.sin_port = htons(atoi(argv[2])+1);

	memset(send_buf, 'A', MAX_TEXT - 1);
	send_buf[MAX_TEXT-1] = '\0';

	cmdSock = socket(AF_INET, SOCK_STREAM, 0);
	connect(cmdSock, (struct sockaddr *) &cmdAddr, addr_size);

	for (int i=0; i<42; i++) {
		datSocks[i] = socket(AF_INET, SOCK_STREAM, 0);
		connect(datSocks[i], (struct sockaddr *) &datAddr, addr_size);
	}

	send(cmdSock, "/reset\n", 7, 0);
	n = read(cmdSock, recv_buf, MAX_TEXT);
	recv_buf[n] = '\0';
	printf("%s", recv_buf);

	while (is_running)
		for (int i=0; i<42; i++)
			send(datSocks[i], send_buf, MAX_TEXT-1, 0);

	// After termination
	send(cmdSock, "/report\n", 8, 0);
	n = read(cmdSock, recv_buf, MAX_TEXT);
	recv_buf[n] = '\0';
	printf("%s", recv_buf);

	for (int i=0; i<42; i++)
		close(datSocks[i]);
}  // main function

void term(int sig) {
	if (sig != SIGTERM && sig != SIGINT) return;
	is_running = false;
}
