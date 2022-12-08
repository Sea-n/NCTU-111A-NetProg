#include "generic.h"

// Size too big
char buf_file[17600100] = {};

int main(int argc, char *argv[]) {
	struct sockaddr_in sin, csin;
	socklen_t sinlen = sizeof(sin);
	char rup_buf[MTU], filename[256];
	RUP *rus = (RUP *) new char[MTU];
	RUM *rum = new RUM;
	RUP *rup = (RUP *) &rup_buf;
	bitset<10010> completed{0};
	timeval tv;
	int idx2file[10010][2] = {{}};
	int file2rem[1001] = {};
	int file2pos[1001] = {};
	long prev, usec;
	int sock, rem;
	__int128 conv;

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

	gettimeofday(&tv, nullptr);
	prev = (tv.tv_sec + 5l) * 1'000'000l + tv.tv_usec;
	memset(rus->content, 0, sizeof(rus->content));
	rus->index = 0;

	for (;;) {  // Main loop
		// Status update
		gettimeofday(&tv, nullptr);
		usec = prev - tv.tv_sec*1'000'000 - tv.tv_usec;
		if (usec < 0) {
			memcpy(rus->content + 1400, &tv, sizeof(tv));
			prev += (rem > 2500) ? 200'000 : 50'000;
			sendto(sock, rus, MTU, 0, (struct sockaddr*) &csin, sinlen);
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
		rem = rum->chunks+2 - completed.count();
		rus->content[rup->index >> 3] |= 1 << (rup->index & 7);

		if (rup->index > 10000) {  // didn't compress metadata
			memcpy((char *) rum + (rup->index - 10001)*CHUNK_PKT, rup->content, CHUNK_PKT);
			if (!completed[10001] || !completed[10002])
				continue;

			int a, b=0, c, d=0;
			for (int k=0; k<1001; k++) {
				a = b; b += rum->size[k];
				c = d; d = b / CHUNK_BUF;
				for (int i=c; i<d; i++)
					idx2file[i][0] = k + 1;
				idx2file[d][1] = k + 1;
				file2rem[k] = d - c + 1;
				file2pos[k] = a;
			}

			continue;
		}

		for (int j=0; j<CHUNK_PKT/14; j++) {  // decompress
			conv = 0;
			for (int i=0; i<14; i++) {
				conv *= 256;
				conv += rup->content[j*14 + i];
			}
			for (int i=0; i<17; i++) {
				buf_file[rup->index*CHUNK_BUF + j*17 + (16-i)] = (conv % 96) + 0x20;
				conv /= 96;
			}
		}

		for (int k=0; k<2; k++) {
			int f = idx2file[rup->index][k] - 1;
			if (f == -1 || --file2rem[f]) continue;
			sprintf(filename, "%s/%06d", argv[1], f);
			ofstream file(filename, ios::binary | ios::out);
			file.write(buf_file + file2pos[f], rum->size[f]);
		}

#ifdef _DEBUG
		gettimeofday(&tv, nullptr);
		if (rem % 500 == 0)
			printf("%02ld.%03ld %20s recv rem=%d\n", tv.tv_sec % 60, tv.tv_usec / 1000, "", rem);
#endif

		// Ready to close client
		if (rem == 6) {
			rus->index = -42;
			for (int i=0; i<10; i++)
				sendto(sock, rus, MTU, 0, (struct sockaddr*) &csin, sinlen);
		}
		if (rem == 0) break;
	}  // End of main loop

	for (int i=0; i<1001; i++)
		if (file2rem[i]) {
			printf("file2rem[%d] = %d\n", i, file2rem[i]);
			sprintf(filename, "%s/%06d", argv[1], i);
			ofstream file(filename, ios::binary | ios::out);
			file.write(buf_file + file2pos[i], rum->size[i]);
		}
	gettimeofday(&tv, nullptr);
	printf("%02ld.%03ld %20s recv stop\n", tv.tv_sec % 60, tv.tv_usec / 1000, "");
}
