#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <netdb.h>

using namespace std;

int main() {
	struct timeval onesec = {1, 0};
	struct sockaddr_in servaddr;
	struct hostent *hostnm;
	char buf[42042];
	int cnt=0, sum=0;
	int sockfd, n;

	// Init
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(10002);
	hostnm = gethostbyname("inp111.zoolab.org");
	servaddr.sin_addr.s_addr = *((unsigned long *) hostnm->h_addr);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		cerr << "Socket error.\n";
		return 1;
	}

	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		cerr << "Connect error.\n";
		return 1;
	}

	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &onesec, sizeof onesec);

	// Connected
	send(sockfd, "GO\n", 3, 0);
	while ( (n = read(sockfd, buf, 42000)) > 0 ) {
		sum += n;
		cnt++;
	}
	cout << "Total: " << cnt << " Packets / " << sum << " Bytes\n";

	// Answer
	sprintf(buf, "%d\n", sum - 173);
	send(sockfd, buf, strlen(buf), 0);

	n = read(sockfd, buf, 42000);
	buf[n] = '\0';
	cout << '\n' << buf << '\n';
}
