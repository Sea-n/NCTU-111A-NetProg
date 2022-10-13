#include <sys/socket.h>
#include <sys/time.h>
#include <iostream>
#include <unistd.h>
#include <netdb.h>

using namespace std;

int main(int argc, char *argv[]) {
	struct timeval now;
	struct sockaddr_in servaddr;
	struct hostent *hostnm;
	char buf[1002003];
	int sockfd, sz, rem=0, n=0;
	int N = 100 * 1000;

	if (argc != 2) {
		cerr << "Usage: " << argv[0] << " <speed>  # MBps\n";
		return 1;
	}

	// Init
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(10003);
	// hostnm = gethostbyname("inp111.zoolab.org");
	hostnm = gethostbyname("localhost");
	servaddr.sin_addr.s_addr = *((unsigned long *) hostnm->h_addr);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		cerr << "Socket error.\n";
		return 1;
	}

	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		cerr << "Connect error.\n";
		return 1;
	}

	// Main
	fill_n(buf, 1002001, 'e');
	// sz = stof(argv[1]) * 0.9963 * N;  // For external
	sz = stof(argv[1]) * 0.9963 * N;  // For local sink server
	cout << "Size: " << sz << endl;
	for (;;) {
		gettimeofday(&now, nullptr);
		usleep(N - now.tv_usec % N);

		n = send(sockfd, buf, sz, 0);
		cout << "Sent " << n << " bytes.\n";
	}
}
