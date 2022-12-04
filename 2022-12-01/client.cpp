#include "client.h"

// Size too big
char buf_ruf[33001002] = {};
RUP rup_chunks[24600];

int main(int argc, char *argv[]) {
	int sock;
	struct sockaddr_in sin, csin;
	socklen_t csinlen = sizeof(csin);
	unsigned char buf_recv[MTU];
	bitset<24600> completed = {0};
	struct timeval zero = {0, 0};
	int repeat = 1;
	fd_set fds;

	if (argc != 5) {
		fprintf(stderr, "Usage: %s <path-to-read-files> <total-number-of-files> <port> <server-ip-address>\n", argv[0]);
		exit(0);
	}

	// Socket init
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[3], NULL, 0));
	inet_pton(AF_INET, argv[4], &sin.sin_addr);
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	// Merge all files into one RUF
	char filename[256];
	RUF *ruf = (RUF *) &buf_ruf;
	char *p = ruf->content;
	for (int k=0; k<1000; k++) {
		sprintf(filename, "%s/%06d", argv[1], k);
		ifstream file(filename, ios::binary | ios::ate);
		ruf->size[k] = file.tellg();
		file.seekg(0, ios::beg);
		file.read(p, ruf->size[k]);
		p += ruf->size[k];
	}

	// Split into RUP chunks
	ruf->chunks = (p - (char *) ruf) / CHUNK + 1;
	for (int k=0; k<ruf->chunks; k++) {
		rup_chunks[k].index = k;
		memcpy(rup_chunks[k].content, buf_ruf + k*CHUNK, CHUNK);
		rup_chunks[k].crc32 = crc32(&rup_chunks[k]);
	}
	printf("Splitted into %d chunks.\n", ruf->chunks);

	for (int k=0; k<3; k++)
		for (int i=0; i<10; i++)
			sendto(sock, &rup_chunks[k], MTU, 0, (struct sockaddr*) &sin, sizeof(sin));

	for (;;) {
		for (int k=0; k<ruf->chunks; k++)
			if (!completed[k])
				for (int r=0; r<repeat; r++) {
					sendto(sock, &rup_chunks[k], MTU, 0, (struct sockaddr*) &sin, sizeof(sin));
					usleep(200);
				}

		FD_ZERO(&fds); FD_SET(sock, &fds);
		while (select(1024, &fds, NULL, NULL, &zero) > 0) {
			RUP *rup_recv = (RUP *) buf_recv;
			recvfrom(sock, buf_recv, MTU, 0, (struct sockaddr*) &csin, &csinlen);
			if (rup_recv->crc32 != crc32(rup_recv)) continue;
			if (rup_recv->index == -87) exit(0);
			if (rup_recv->index < 0) continue;
			for (int j=0; j<CHUNK; j++)
				for (int i=0; i<8; i++)
					completed[(rup_recv->index * CHUNK + j) * 8 + i] = (rup_recv->content[j] & (1<<i)) ? 1 : 0;
		}

		// printf("Completed %ld / %d  rep=%d\n", completed.count(), ruf->chunks, repeat);

		if (ruf->chunks - completed.count() < 20500)
			repeat = 1;
		else if (ruf->chunks - completed.count() < 1000)
			repeat = 2;
		else if (ruf->chunks - completed.count() < 500)
			repeat = 3;
		else if (ruf->chunks - completed.count() < 200)
			repeat = 4;
	}

	close(sock);
}
