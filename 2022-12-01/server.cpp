#include "server.h"

int main(int argc, char *argv[]) {
	int sock;
	struct sockaddr_in sin;

	if (argc != 4) {
		fprintf(stderr, "usage: %s <path-to-store-files> <total-number-of-files> <port>\n", argv[0]);
		exit(0);
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[argc-1], NULL, 0));

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	bind(sock, (struct sockaddr*) &sin, sizeof(sin));

	while (1) {
		struct sockaddr_in csin;
		socklen_t csinlen = sizeof(csin);
		unsigned char buf[MTU];
		int rlen;
		
		if ((rlen = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*) &csin, &csinlen)) < 0) {
			perror("recvfrom");
			continue;
		}
		RUP *p = (RUP *) buf;

		printf("Recv %d\n", rlen);
		printf("Type=%x idx=%d frag=%d\\%d CRC32=%x\n", p->type, p->index, p->frag_cnt, p->frag_seq, p->crc32);
		printf("Content %s\n", p->content);

		sendto(sock, p, rlen, 0, (struct sockaddr*) &csin, sizeof(csin));
	}

	close(sock);
}
