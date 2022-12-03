#include "client.h"

static int sock = -1;
static struct sockaddr_in sin;
static unsigned seq;
static unsigned count = 0;

int main(int argc, char *argv[]) {
	if (argc != 5) {
		fprintf(stderr, "Usage: %s <path-to-read-files> <total-number-of-files> <port> <server-ip-address>\n", argv[0]);
		exit(1);
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	inet_pton(AF_INET, argv[4], &sin.sin_addr);
	sin.sin_port = htons(strtol(argv[3], NULL, 0));

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	for (;;) {
		int rlen;
		struct sockaddr_in csin;
		socklen_t csinlen = sizeof(csin);
		char buf[2048];

		RUP *p = (RUP*) buf;
		p->type = TYPE_DIR_LIST_GET;
		p->index = 0;
		p->frag_cnt = 1;
		p->frag_seq = 0;
		crc32_calc(p);
		strcpy(p->content, "Meow");
		sendto(sock, p, sizeof(*p), 0, (struct sockaddr*) &sin, sizeof(sin));
		
		if ((rlen = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*) &csin, &csinlen)) < 0) {
			perror("recvfrom");
			continue;
		}

		break;
	}

	close(sock);
}
