#include "generic.h"

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
	fd_set fds;
	timeval now, tv;
	long prev;

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

	gettimeofday(&now, nullptr);
	prev = now.tv_sec * 1000'000 + now.tv_usec - 100'000;
	int rem;
	long usec;
	for (;;) {
		for (int k=0; k<ruf->chunks; k++) {
			if (!completed[k]) {
				prev += (rem > 200) ? 740 : 1000;
				gettimeofday(&now, nullptr);
				usec = prev - now.tv_sec*1000'000 - now.tv_usec;
				if (usec > 0)
					usleep(usec);

				sendto(sock, &rup_chunks[k], MTU, 0, (struct sockaddr*) &sin, sizeof(sin));
			}

			FD_ZERO(&fds); FD_SET(sock, &fds);
			if (select(1024, &fds, NULL, NULL, &zero) <= 0)
				continue;

			RUP *rup_recv = (RUP *) buf_recv;
			recvfrom(sock, buf_recv, MTU, 0, (struct sockaddr*) &csin, &csinlen);
			if (rup_recv->crc32 != crc32(rup_recv)) continue;
			if (rup_recv->index == -87) exit(0);
			if (rup_recv->index < 0) continue;
			for (int j=0; j<CHUNK; j++)
				for (int i=0; i<8; i++)
					completed[(rup_recv->index * CHUNK + j) * 8 + i] = (rup_recv->content[j] & (1<<i)) ? 1 : 0;

			rem = ruf->chunks - completed.count();
			gettimeofday(&tv, nullptr);
#ifdef _DEBUG
			printf("%03ld.%03ld  send rem=%d\n", tv.tv_sec % 1000, tv.tv_usec / 1000, rem);
#endif
		}
	}
}
