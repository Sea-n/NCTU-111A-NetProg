#include "generic.h"

// Size too big
char rup_buf[MTU], ruf_buf[33001002] = {};

int main(int argc, char *argv[]) {
	struct sockaddr_in sin, csin;
	socklen_t sinlen = sizeof(sin);
	RUP *rus = (RUP *) new char[MTU];
	RUF *ruf = (RUF *) &ruf_buf;
	RUP *rup = (RUP *) &rup_buf;
	bitset<24600> completed{0};
	timeval tv, now;
	long prev, usec;
	int sock, rem;

	if (argc != 4) {
		fprintf(stderr, "usage: %s <path-to-store-files> <total-number-of-files> <port>\n", argv[0]);
		exit(1);
	}

	// Socket init
	memset(&sin, 0, sinlen);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[3], NULL, 0));
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	bind(sock, (struct sockaddr*) &sin, sinlen);

	gettimeofday(&now, nullptr);
	prev = (now.tv_sec + 6l) * 1'000'000l + now.tv_usec;

	for (;;) {  // Main loop
		// Status update
		gettimeofday(&now, nullptr);
		usec = prev - now.tv_sec*1'000'000 - now.tv_usec;
		if (usec < 0) {
			prev += (rem > 2000) ? 500'000 : 100'000;
			for (int k=(ruf->chunks / CHUNK / 8); k>=0; k--) {
				rus->index = k;
				for (int j=0; j<CHUNK; j++) {
					rus->content[j] = 0;
					for (int i=0; i<8; i++)
						rus->content[j] |= completed[(k * CHUNK + j) * 8 + i] ? (1<<i) : 0;
				}
				sendto(sock, rus, MTU, 0, (struct sockaddr*) &csin, sinlen);
			}
		}

		// Receive packets
		fd_set fds;
		timeval zero = {0, 0};
		FD_ZERO(&fds); FD_SET(sock, &fds);
		if (select(1024, &fds, NULL, NULL, &zero) <= 0)
			continue;

		recvfrom(sock, rup_buf, MTU, 0, (struct sockaddr*) &csin, &sinlen);
		if (completed.test(rup->index)) continue;  // Duplicated
		memcpy(ruf_buf + rup->index*CHUNK, rup->content, CHUNK);
		completed.set(rup->index);
		rem = ruf->chunks - completed.count();
		gettimeofday(&tv, nullptr);
#ifdef _DEBUG
		if (rem % 500 == 0 || (rem < 1000 && rem % 50 == 0))
			printf("%02ld.%03ld %20s recv rem=%d\n", tv.tv_sec % 60, tv.tv_usec / 1000, "", rem);
#endif
		if (rem == 0) break;
	}  // End of main loop

	char filename[256];
	char *p = ruf->content;
	for (int k=0; k<1000; k++) {
		sprintf(filename, "%s/%06d", argv[1], k);
		ofstream file(filename, ios::binary | ios::out);
		file.write(p, ruf->size[k]);
		p += ruf->size[k];
	}

	printf("%02ld.%03ld %20s recv completed.\n", tv.tv_sec % 60, tv.tv_usec / 1000, "");
}
