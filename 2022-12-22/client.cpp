#include "generic.h"

char buf_file[32001002];

unsigned short csum(unsigned short *buf, int nwords) {
	unsigned long sum = 0;
	while (nwords--) sum += *buf++;
	sum = (sum >> 16) + (sum &0xffff);
	sum += (sum >> 16);
	return (unsigned short) ~sum;
}

int main(int argc, char *argv[]) {
	struct sockaddr_in sin;
	socklen_t sinlen = sizeof(sin);
	char *ruf = buf_file + sizeof(RUM), *p = ruf;
	char buf_pkt[1500], filename[256];
	iphdr *ip_hdr = (iphdr *) buf_pkt;
	RUP *rup = (RUP *) (buf_pkt + 20);
	RUM *rum = (RUM *) buf_file;
	int sock, one=1;

	if (argc != 4) {
		fprintf(stderr, "Usage: %s <path-to-read-files> <total-number-of-files> <boardcast-address>\n", argv[0]);
		exit(1);
	}

	// Socket init
	memset(&sin, 0, sinlen);
	sin.sin_family = AF_INET;
	inet_pton(AF_INET, argv[3], &sin.sin_addr);
	sock = socket(AF_INET, SOCK_RAW, 42);
	setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(int));
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &one, sizeof(int));

	// Fill header with custom source IP address
	ip_hdr->ihl = 5;
	ip_hdr->version = 4;
	ip_hdr->tot_len = MTU;
	ip_hdr->protocol = 42;
	ip_hdr->daddr = inet_addr(argv[3]);
	ip_hdr->saddr = inet_addr("10.113.113.42");

	// Merge all files into one buffer
	for (int k=0; k<1000; k++) {
		sprintf(filename, "%s/%06d", argv[1], k);
		ifstream file(filename, ios::binary | ios::ate);
		rum->size[k] = file.tellg();
		file.seekg(0, ios::beg);
		file.read(p, rum->size[k]);
		p += rum->size[k];
	}
	rum->chunks = (p - (char *) rum) / CHUNK + 1;
	printf("Total %d chunks.\n", rum->chunks);

	// Split into RUP chunks
	for (int k=0; k<rum->chunks; k++) {
		rup->index = k;
		memcpy(rup->content, buf_file + k*CHUNK, CHUNK);
		ip_hdr->check = csum((unsigned short *) buf_pkt, sizeof(struct iphdr));
		sendto(sock, buf_pkt, MTU, 0, (struct sockaddr*) &sin, sinlen);
		usleep(1);
	}

	// Wait for server store files
	usleep(100'000);
	return 0;
}
