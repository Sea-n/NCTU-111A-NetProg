#include "generic.h"

// Size too big
char buf_ruf[33001002] = {};
RUP rup[24600];

int main(int argc, char *argv[]) {
	struct sockaddr_in sin, csin;
	socklen_t sinlen = sizeof(sin);
	timeval tv, zero = {0, 0};
	bitset<24600> completed = {0};
	char rus_buf[MTU];
	RUF *ruf = (RUF *) &buf_ruf;
	RUP *rus = (RUP *) rus_buf;
	long prev, usec;
	int sock, rem;
	fd_set fds;

	if (argc != 5) {
		fprintf(stderr, "Usage: %s <path-to-read-files> <total-number-of-files> <port> <server-ip-address>\n", argv[0]);
		exit(0);
	}
	gettimeofday(&tv, nullptr);
	printf("%02ld.%03ld  send begin\n", tv.tv_sec % 60, tv.tv_usec / 1000);

	// Socket init
	memset(&sin, 0, sinlen);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[3], NULL, 0));
	inet_pton(AF_INET, argv[4], &sin.sin_addr);
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	// Merge all files into one RUF
	char filename[256];
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
	rem = ruf->chunks;
	for (int k=0; k<ruf->chunks; k++) {
		rup[k].index = k;
		memcpy(rup[k].content, buf_ruf + k*CHUNK, CHUNK);
		rup[k].crc32 = crc32(&rup[k]);
	}

	// Send metadata first
	printf("%02ld.%03ld  Total %d chunks\n", tv.tv_sec % 60, tv.tv_usec / 1000, rem);
	for (int i=0; i<10; i++)
		sendto(sock, &rup[0], MTU, 0, (struct sockaddr*) &sin, sinlen);

	gettimeofday(&tv, nullptr);
	prev = tv.tv_sec * 1'000'000l + tv.tv_usec - 360'000l;
	for (int round=0; round<24; round++) {  // Main loop
		for (int k=0; k<ruf->chunks; k++) {
			// Send packets
			while (completed[k]) k++;
			gettimeofday(&tv, nullptr);
			usec = prev - tv.tv_sec*1'000'000l - tv.tv_usec;
			if (usec < 0) {
				prev += (rem > 2000) ? 745 : 800;
				sendto(sock, &rup[k], MTU, 0, (struct sockaddr*) &sin, sinlen);
			} else {
				k--;
			}

			// Update status
			FD_ZERO(&fds); FD_SET(sock, &fds);
			if (select(1024, &fds, NULL, NULL, &zero) <= 0)
				continue;

			recvfrom(sock, rus_buf, MTU, 0, (struct sockaddr*) &csin, &sinlen);
			if (rus->crc32 != crc32(rus)) continue;
			if (rus->index < 0) continue;
			for (int j=0; j<CHUNK; j++)
				for (int i=0; i<8; i++)
					completed[(rus->index * CHUNK + j) * 8 + i] = (rus->content[j] & (1<<i)) ? 1 : 0;

			rem = ruf->chunks - completed.count();
#ifdef _DEBUG
			gettimeofday(&tv, nullptr);
			printf("%02ld.%03ld  send rem=%d\n", tv.tv_sec % 60, tv.tv_usec / 1000, rem);
#endif
		}

		gettimeofday(&tv, nullptr);
#ifdef _DEBUG
		printf("%02ld.%03ld  send round=%d\n", tv.tv_sec % 60, tv.tv_usec / 1000, round);
#endif
	}  // End of main loop
}
