#include "generic.h"

// Size too big
char buf_file[17600100] = {};
RUP rup[10010];

int main(int argc, char *argv[]) {
	struct sockaddr_in sin, csin;
	socklen_t sinlen = sizeof(sin);
	timeval tv, now, zero = {0, 0};
	bitset<10010> completed = {0};
	char rus_buf[MTU];
	RUM *rum = new RUM;
	RUP *rus = (RUP *) rus_buf;
	__int128 conv;
	long prev, usec, delay=730;
	int sock, rem;
	fd_set fds;

	if (argc != 5) {
		fprintf(stderr, "Usage: %s <path-to-read-files> <total-number-of-files> <port> <server-ip-address>\n", argv[0]);
		exit(0);
	}
	gettimeofday(&now, nullptr);
	printf("%02ld.%03ld  send begin\n", now.tv_sec % 60, now.tv_usec / 1000);

	// Socket init
	memset(&sin, 0, sinlen);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[3], NULL, 0));
	inet_pton(AF_INET, argv[4], &sin.sin_addr);
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	// Merge all files into one buffer
	char filename[256];
	char *p = buf_file;
	for (int k=0; k<1000; k++) {
		sprintf(filename, "%s/%06d", argv[1], k);
		ifstream file(filename, ios::binary | ios::ate);
		rum->size[k] = file.tellg();
		file.seekg(0, ios::beg);
		file.read(p, rum->size[k]);
		p += rum->size[k];
	}
	rum->size[1000] = CHUNK_BUF;
	memset(p, 'A', CHUNK_BUF);
	p += CHUNK_BUF;

	// Send metadata first
	rum->chunks = (p - buf_file) / CHUNK_BUF + 1;
	rem = rum->chunks+2;
	gettimeofday(&now, nullptr);
	printf("%02ld.%03ld  Total %d chunks\n", now.tv_sec % 60, now.tv_usec / 1000, rem);
	for (int k=0; k<2; k++) {
		rup[10001 + k].index = 10001 + k;
		memcpy(rup[10001 + k].content, (char *) rum + k*CHUNK_PKT, CHUNK_PKT);
		for (int i=0; i<10; i++)
			sendto(sock, &rup[10001 + k], MTU, 0, (struct sockaddr*) &sin, sinlen);
	}

	// Split into RUP chunks
	gettimeofday(&now, nullptr);
	prev = now.tv_sec * 1'000'000l + now.tv_usec - 320'000l;
	printf("%02ld.%03ld  round=1\n", now.tv_sec % 60, now.tv_usec / 1000);
	for (int k=0; k<rum->chunks; k++) {  // file content: 0x21 - 0x7E
		rup[k].index = k;
		for (int j=0; j<CHUNK_PKT/14; j++) {
			conv = 0;
			for (int i=0; i<17; i++) {
				conv *= 96;
				conv += (buf_file[k*CHUNK_BUF + j*17 + i] - 0x20) % 96;
			}
			for (int i=0; i<14; i++) {
				rup[k].content[j*14 + (13-i)] = conv % 256;
				conv /= 256;
			}
		}

		gettimeofday(&now, nullptr);
		usec = prev - now.tv_sec*1'000'000l - now.tv_usec;
		if (usec > 0)
			usleep(usec);
		prev += delay;
		sendto(sock, &rup[k], MTU, 0, (struct sockaddr*) &sin, sinlen);
	}

	for (int round=2; ; round++) {  // Main loop
		gettimeofday(&now, nullptr);
		if (round < 12)
			printf("%02ld.%03ld  round=%d\n", now.tv_sec % 60, now.tv_usec / 1000, round);
		for (int k=0; k<rum->chunks; k++) {
			// Send packets
			while (completed[k]) k++;
			gettimeofday(&now, nullptr);
			usec = prev - now.tv_sec*1'000'000l - now.tv_usec;
			if (usec < 0) {
				prev += delay;
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
			gettimeofday(&now, nullptr);
			memcpy(&tv, rus->content + 1400, sizeof(tv));
			usec = (now.tv_sec - tv.tv_sec) * 1'000'000l + (now.tv_usec - tv.tv_usec);
			if (usec > 300'000) delay = 900;
			else if (usec > 200'000) delay = 800;
			else if (usec > 155'000) delay = 760;
			else if (usec > 145'000) delay = 755;
			else if (usec > 130'000) delay = 750;
			else delay = 200;
			for (int j=0; j<1280; j++)
				for (int i=0; i<8; i++)
					if (rus->content[j] & (1<<i))
						completed.set((rus->index * CHUNK_PKT + j) * 8 + i);

			rem = rum->chunks+2 - completed.count();
			gettimeofday(&tv, nullptr);
			printf("%02ld.%03ld  send rem=%d  usec=%ld\n", tv.tv_sec % 60, tv.tv_usec / 1000, rem, usec / 100);
#ifdef _DEBUG
#endif
		}

		gettimeofday(&tv, nullptr);
	}  // End of main loop
}
