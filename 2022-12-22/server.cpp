#include "generic.h"

char buf_file[32001002];

int main(int argc, char *argv[]) {
	struct sockaddr_in sin, csin;
	socklen_t sinlen = sizeof(sin);
	char buf_pkt[1500], filename[256];
	char *p = buf_file + sizeof(RUM);
	RUP *rup = (RUP *) (buf_pkt + 20);
	RUM *rum = (RUM *) buf_file;
	int sock, one=1;

	if (argc != 4) {
		fprintf(stderr, "usage: %s <path-to-store-files> <total-number-of-files> <boardcast-address>\n", argv[0]);
		exit(1);
	}

	// Socket init
	memset(&sin, 0, sinlen);
	sin.sin_family = AF_INET;
	inet_pton(AF_INET, argv[3], &sin.sin_addr);
	sock = socket(AF_INET, SOCK_RAW, 42);
	bind(sock, (struct sockaddr*) &sin, sinlen);
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &one, sizeof(int));

	// Receive and write files
	for (int k=0; k<10'000 || k<rum->chunks; k++) {
		recvfrom(sock, buf_pkt, MTU, 0, (struct sockaddr*) &csin, &sinlen);
		memcpy(buf_file + rup->index*CHUNK, rup->content, CHUNK);
	}

	for (int k=0; k<1000; k++) {
		sprintf(filename, "%s/%06d", argv[1], k);
		ofstream file(filename, ios::binary | ios::out);
		file.write(p, rum->size[k]);
		p += rum->size[k];
	}

	return 0;
}
