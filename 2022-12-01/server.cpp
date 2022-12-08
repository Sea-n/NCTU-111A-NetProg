#include "generic.h"

// Size too big
char buf_file[17600100] = {};

int main(int argc, char *argv[]) {
	struct sockaddr_in sin, csin;
	socklen_t sinlen = sizeof(sin);
	char rup_buf[MTU], filename[256];
	RUP *rus = (RUP *) new char[MTU];
	RUF *ruf = new RUF;
	RUP *rup = (RUP *) &rup_buf;
	bitset<24600> completed{0};
	timeval tv, now;
	unsigned long prev, conv;
	long usec;
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
	prev = (now.tv_sec + 5l) * 1'000'000l + now.tv_usec;
	memset(rus->content, 0, sizeof(rus->content));
	rus->index = 0;

	for (;;) {  // Main loop
		// Status update
		gettimeofday(&now, nullptr);
		usec = prev - now.tv_sec*1'000'000 - now.tv_usec;
		if (usec < 0) {
			prev += (rem > 800) ? 300'000 : 50'000;
			sendto(sock, rus, MTU, 0, (struct sockaddr*) &csin, sinlen);
#ifdef _DEBUG
			gettimeofday(&tv, nullptr);
			printf("%02ld.%03ld %20s recv rem=%d\n", tv.tv_sec % 60, tv.tv_usec / 1000, "", rem);
#endif
		}

		// Receive packets
		fd_set fds;
		timeval zero = {0, 0};
		FD_ZERO(&fds); FD_SET(sock, &fds);
		if (select(10, &fds, NULL, NULL, &zero) <= 0)
			continue;

		recvfrom(sock, rup_buf, MTU, 0, (struct sockaddr*) &csin, &sinlen);
		if (completed.test(rup->index)) continue;  // Duplicated
		completed.set(rup->index);
		rus->content[rup->index >> 3] |= 1 << (rup->index & 7);

		if (rup->index < 2) {  // didn't compress metadata
			memcpy((char *)ruf + rup->index*CHUNK_PKT, rup->content, CHUNK_PKT);
			continue;
		}

		for (int j=0; j<CHUNK_PKT/5; j++) {  // decompress
			conv = 0;
			for (int i=0; i<5; i++) {
				conv *= 256;
				conv += rup->content[j*5 + i];
			}
			for (int i=0; i<6; i++) {
				buf_file[rup->index*CHUNK_BUF + j*6 + (5-i)] = (conv % 96) + 0x20;
				conv /= 96;
			}
		}

		rem = ruf->chunks - completed.count();
#ifdef _DEBUG
		gettimeofday(&tv, nullptr);
		if (rem % 500 == 0)
			printf("%02ld.%03ld %20s recv rem=%d\n", tv.tv_sec % 60, tv.tv_usec / 1000, "", rem);
#endif
		if (rem == 0) break;
	}  // End of main loop

	char *p = buf_file;
	for (int k=0; k<1000; k++) {
		sprintf(filename, "%s/%06d", argv[1], k);
		ofstream file(filename, ios::binary | ios::out);
		file.write(p, ruf->size[k]);
		p += ruf->size[k];
	}

	// Close client
	rus->index = -42;
	for (int i=0; i<10; i++)
		sendto(sock, rus, MTU, 0, (struct sockaddr*) &csin, sinlen);

	gettimeofday(&tv, nullptr);
	printf("%02ld.%03ld %20s recv written.\n", tv.tv_sec % 60, tv.tv_usec / 1000, "");
}
