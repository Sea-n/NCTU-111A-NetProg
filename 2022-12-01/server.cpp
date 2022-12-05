#include "generic.h"

// Size too big
char ruf_buf[33001002] = {};

int main(int argc, char *argv[]) {
	int sock;
	struct sockaddr_in sin, csin;
	socklen_t csinlen = sizeof(csin);
	struct timeval tv, now, prev;
	char recv_buf[MTU];
	RUF *ruf = (RUF *) &ruf_buf;

	if (argc != 4) {
		fprintf(stderr, "usage: %s <path-to-store-files> <total-number-of-files> <port>\n", argv[0]);
		exit(1);
	}

	// Socket init
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[3], NULL, 0));
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	bind(sock, (struct sockaddr*) &sin, sizeof(sin));

	RUP *rup_recv = (RUP *) &recv_buf;
	recvfrom(sock, recv_buf, MTU, 0, (struct sockaddr*) &csin, &csinlen);

	bitset<24600> completed{0};


	for (;;) {
		prev = now;
		gettimeofday(&now, nullptr);
		if (now.tv_usec % 80000 < prev.tv_usec % 80000) {
			RUP *rus = (RUP *) new char[MTU];
			for (int k=(ruf->chunks / CHUNK / 8); k>=0; k--) {
				rus->index = k;
				for (int j=0; j<CHUNK; j++) {
					rus->content[j] = 0;
					for (int i=0; i<8; i++)
						rus->content[j] |= completed[(k * CHUNK + j) * 8 + i] ? (1<<i) : 0;
				}
				rus->crc32 = crc32(rus);
				sendto(sock, rus, MTU, 0, (struct sockaddr*) &csin, sizeof(csin));
			}
		}

		recvfrom(sock, recv_buf, MTU, 0, (struct sockaddr*) &csin, &csinlen);
		if (rup_recv->crc32 != crc32(rup_recv)) continue;
		if (completed.test(rup_recv->index)) continue;
		memcpy(ruf_buf + rup_recv->index*CHUNK, rup_recv->content, CHUNK);
		completed.set(rup_recv->index);
		gettimeofday(&tv, nullptr);
#ifdef _DEBUG
		if (completed.count() % 50 == 0 && 1)
			printf("%03ld.%03ld %20s recv rem=%ld\n", tv.tv_sec % 1000, tv.tv_usec / 1000,
					"", ruf->chunks - completed.count());
#endif
		if ((int) completed.count() == ruf->chunks) break;
	}
	printf("%03ld.%03ld %20s recv completed.\n", tv.tv_sec % 1000, tv.tv_usec / 1000, "");

	RUP *rus = (RUP *) new char[MTU];
	for (int k=0; k<3; k++) {
		rus->index = -87;
		rus->crc32 = crc32(rus);
		for (int i=0; i<10; i++) {
			sendto(sock, rus, MTU, 0, (struct sockaddr*) &csin, sizeof(csin));
			usleep(100);
		}
	}

	char filename[256];
	char *p = ruf->content;
	for (int k=0; k<1000; k++) {
		sprintf(filename, "%s/%06d", argv[1], k);
		ofstream file(filename, ios::binary | ios::out);
		file.write(p, ruf->size[k]);
		p += ruf->size[k];
	}
}
