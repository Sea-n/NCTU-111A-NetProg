#include "generic.h"

// Size too big
char buf_file[17600100] = {};
RUP rup[9800];

int main(int argc, char *argv[]) {
	struct sockaddr_in sin, csin;
	socklen_t sinlen = sizeof(sin);
	timeval tv, zero = {0, 0};
	bitset<24600> completed = {0};
	char rus_buf[MTU];
	RUF *ruf = new RUF;
	RUP *rus = (RUP *) rus_buf;
	unsigned long prev, conv;
	long usec;
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
	char *p = buf_file;
	for (int k=0; k<1000; k++) {
		sprintf(filename, "%s/%06d", argv[1], k);
		ifstream file(filename, ios::binary | ios::ate);
		ruf->size[k] = file.tellg();
		file.seekg(0, ios::beg);
		file.read(p, ruf->size[k]);
		p += ruf->size[k];
	}

	// Split into RUP chunks
	ruf->chunks = (p - (char *) buf_file) / CHUNK_BUF + 1;
	rem = ruf->chunks;
	for (int k=0; k<2; k++) {  // metadata: full range
		rup[k].index = k;
		memcpy(rup[k].content, (char *) ruf + k*CHUNK_PKT, CHUNK_PKT);
	}
	for (int k=2; k<ruf->chunks; k++) {  // file content: 0x21 - 0x7E
		rup[k].index = k;
		for (int j=0; j<CHUNK_PKT/5; j++) {
			conv = 0;
			for (int i=0; i<6; i++) {
				conv *= 96;
				conv += buf_file[k*CHUNK_BUF + j*6 + i] - 0x20;
			}
			for (int i=0; i<5; i++) {
				rup[k].content[j*5 + (4-i)] = conv % 256;
				conv /= 256;
			}
		}
	}

	// Send metadata first
	printf("%02ld.%03ld  Total %d chunks\n", tv.tv_sec % 60, tv.tv_usec / 1000, rem);
	for (int k=0; k<2; k++)
		for (int i=0; i<10; i++)
			sendto(sock, &rup[k], MTU, 0, (struct sockaddr*) &sin, sinlen);

	gettimeofday(&tv, nullptr);
	prev = tv.tv_sec * 1'000'000l + tv.tv_usec - 420'000l;
	for (int round=0; ; round++) {  // Main loop
		for (int k=0; k<ruf->chunks; k++) {
			// Send packets
			while (completed[k]) k++;
			gettimeofday(&tv, nullptr);
			usec = prev - tv.tv_sec*1'000'000l - tv.tv_usec;
			if (usec < 0) {
				prev += (round < 10) ? 754 : 800;
				sendto(sock, &rup[k], MTU, 0, (struct sockaddr*) &sin, sinlen);
			} else {
				k--;
			}

			// Update status
			FD_ZERO(&fds); FD_SET(sock, &fds);
			if (select(10, &fds, NULL, NULL, &zero) <= 0)
				continue;

			recvfrom(sock, rus_buf, MTU, 0, (struct sockaddr*) &csin, &sinlen);
			if (rus->index == -42) return 0;
			for (int j=0; j<CHUNK_PKT; j++)
				for (int i=0; i<8; i++)
					if (rus->content[j] & (1<<i))
						completed.set((rus->index * CHUNK_PKT + j) * 8 + i);

			rem = ruf->chunks - completed.count();
#ifdef _DEBUG
			gettimeofday(&tv, nullptr);
			printf("%02ld.%03ld  send rem=%d\n", tv.tv_sec % 60, tv.tv_usec / 1000, rem);
#endif
		}

		gettimeofday(&tv, nullptr);
#ifdef _DEBUG
		if (round < 20)
			printf("%02ld.%03ld  round=%d\n", tv.tv_sec % 60, tv.tv_usec / 1000, round);
#endif
	}  // End of main loop
}
