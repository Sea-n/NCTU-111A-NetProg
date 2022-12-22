#include "dns.h"

string fqdn2name(const char *fqdn);
string name2fqdn(const char *name);

int main(int argc, char *argv[]) {
	struct sockaddr_in sin, csin;
	socklen_t sinlen = sizeof(sin);
	char rcv_buf[MTU], snd_buf[MTU];
	DNS *rcv = (DNS *) rcv_buf;
	DNS *snd = (DNS *) snd_buf;
	int sock;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <port> <config>\n", argv[0]);
		exit(1);
	}

	// Socket init
	memset(&sin, 0, sinlen);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(strtol(argv[1], NULL, 0));
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	bind(sock, (struct sockaddr*) &sin, sinlen);

	// Construct RR set
	char fwd_ns[64], line1[512], line2[512], *p2;
	char *ptr1, *ptr2;
	string dn, fn;
	vector<ZONE> zones;
	ifstream conf("config.txt");
	conf >> fwd_ns;
	printf("Name Server: %s\n", fwd_ns);
	while (conf >> line1) {
		vector<RR> rrs;
		dn = strtok_r(line1, ",", &ptr1);
		fn = strtok_r(nullptr, "", &ptr1);
		ifstream zf(fn);
		zf.getline(line2, 512);
		while (zf.getline(line2, 512)) {
			RR rr;
			rr.name = strtok_r(line2, ",", &ptr2);
			rr.ttl = atoi(strtok_r(nullptr, ",", &ptr2));
			rr.rdlen = 0;

			p2 = strtok_r(nullptr, ",", &ptr2);
			if (strcmp(p2, "IN") == 0) rr.clazz = 1;
			if (strcmp(p2, "CS") == 0) rr.clazz = 2;
			if (strcmp(p2, "CH") == 0) rr.clazz = 3;
			if (strcmp(p2, "HS") == 0) rr.clazz = 4;

			p2 = strtok_r(nullptr, ",", &ptr2);
			if (strcmp(p2, "A") == 0) {
				rr.type = 1;
				unsigned int *ui = (unsigned int *) rr.rdata;
				*ui = inet_addr(strtok_r(nullptr, "", &ptr2));  // Address
				rr.rdlen += 4;
			}
			if (strcmp(p2, "NS") == 0) {
				rr.type = 2;
				strcpy(rr.rdata, fqdn2name(strtok_r(nullptr, "\r\n ", &ptr2)).c_str());
				rr.rdlen += strlen(rr.rdata);  // NSDNAME
				rr.rdata[rr.rdlen++] = '\0';
			}
			if (strcmp(p2, "CNAME") == 0) {
				rr.type = 5;
				unsigned int *ui = (unsigned int *) rr.rdata;
				*ui = inet_addr(strtok_r(nullptr, "", &ptr2));
			}
			if (strcmp(p2, "SOA") == 0) {
				rr.type = 6;
				strcpy(rr.rdata, fqdn2name(strtok_r(nullptr, " ", &ptr2)).c_str());  // MNAME
				rr.rdlen += strlen(rr.rdata) + 1;
				strcpy(rr.rdata+rr.rdlen, fqdn2name(strtok_r(nullptr, " ", &ptr2)).c_str());  // RNAME
				rr.rdlen += strlen(rr.rdata+rr.rdlen) + 1;

				unsigned int *ui = (unsigned int *) (rr.rdata+rr.rdlen);
				*ui = __builtin_bswap32(atoi(strtok_r(nullptr, " ", &ptr2))); rr.rdlen += 4;  // Serial
				*++ui = __builtin_bswap32(atoi(strtok_r(nullptr, " ", &ptr2))); rr.rdlen += 4;  // Refresh
				*++ui = __builtin_bswap32(atoi(strtok_r(nullptr, " ", &ptr2))); rr.rdlen += 4;  // Retry
				*++ui = __builtin_bswap32(atoi(strtok_r(nullptr, " ", &ptr2))); rr.rdlen += 4;  // Expire
				*++ui = __builtin_bswap32(atoi(strtok_r(nullptr, "", &ptr2))); rr.rdlen += 4;  // Minimum
			}
			if (strcmp(p2, "MX") == 0) {
				rr.type = 15;
				int pref = atoi(strtok_r(nullptr, " ", &ptr2));
				rr.rdata[0] = pref >> 8; rr.rdata[1] = pref & 0xFF;
				rr.rdlen += 2;  // Preference
				strcpy(rr.rdata+2, fqdn2name(strtok_r(nullptr, "\r\n ", &ptr2)).c_str());
				rr.rdlen += strlen(rr.rdata+2);  // Exchange
				rr.rdata[rr.rdlen++] = '\0';
			}
			if (strcmp(p2, "TXT") == 0) {
				rr.type = 16;
				unsigned int *ui = (unsigned int *) rr.rdata;
				*ui = inet_addr(strtok_r(nullptr, "", &ptr2));
			}
			if (strcmp(p2, "AAAA") == 0) {
				rr.type = 28;
				unsigned int *ui = (unsigned int *) rr.rdata;
				*ui = inet_addr(strtok_r(nullptr, "", &ptr2));
			}

			rrs.push_back(rr);
		}
		printf("Line %s / %s\n", dn.c_str(), fn.c_str());
		zones.push_back({dn, rrs});
	}

	for (;;) {  // Main loop
		recvfrom(sock, rcv_buf, MTU, 0, (struct sockaddr*) &csin, &sinlen);
		printf("id=%x, qr=%d, rcode=%d\n", rcv->id, rcv->qr, rcv->rcode);

		char *p = rcv_buf + sizeof(DNS);
		char qname_raw[512];
		strcpy(qname_raw, p);
		string qname = name2fqdn(qname_raw);
		p += qname.size();
		int qtype = p[2];  // Only support up to 255
		int qclass = p[4];

		int snd_len = sizeof(DNS);
		memset(snd_buf, 0, MTU);

		for (ZONE zone : zones) {
			if (qname != zone.F && !qname.ends_with('.' + zone.F))
				continue;

			for (RR rr : zone.S) {
				if (qname == zone.F) {
					if (rr.name != "@") continue;
				} else {
					if (qname != rr.name + '.' + zone.F) continue;
				}
				if (qtype != rr.type) continue;
				if (qclass != rr.clazz) continue;

				printf("Meow %s %s %d  %d\n", rr.name.c_str(), zone.F.c_str(), rr.type, rr.rdlen);

				strcpy(snd_buf+snd_len, qname_raw);
				snd_len += strlen(snd_buf+snd_len);
				snd_buf[snd_len++] = '\0';
				snd_buf[snd_len++] = 0; snd_buf[snd_len++] = rr.type;
				snd_buf[snd_len++] = 0; snd_buf[snd_len++] = rr.clazz;

				int *snd_ttl = (int *) &snd_buf[snd_len];
				*snd_ttl = __builtin_bswap32(rr.ttl); snd_len += 4;

				snd_buf[snd_len++] = rr.rdlen >> 8;
				snd_buf[snd_len++] = rr.rdlen & 0xFF;
				memcpy(snd_buf+snd_len, rr.rdata, rr.rdlen);
				snd_len += rr.rdlen;
			}
			snd->id = rcv->id;
			snd->qr = 1;
			snd->aa = 1;
			snd->ra = 1;
			snd->ancount = 1;
		}
			goto response;

		// Forward

response:
		sendto(sock, snd_buf, snd_len, 0, (struct sockaddr*) &csin, sinlen);
	}
}

string fqdn2name(const char *fqdn) {
	int len = 0;
	char name[512], buf[512];
	strcpy(buf, fqdn);
	char *p = strtok(buf, ".");
	name[0] = strlen(p);
	strcpy(name+1, p);
	len += strlen(p) + 1;
	while ((p = strtok(nullptr, ".")) != nullptr) {
		name[len] = strlen(p);
		strcpy(name+len+1, p);
		len += strlen(p) + 1;
	}
	name[len] = '\0';
	return name;
}

string name2fqdn(const char *name) {
	const char *p = name;
	int len = 0;
	char fqdn[512];
	while (p[0]) {
		strncpy(&fqdn[len], &p[1], (int) p[0]);
		len += p[0] + 1;
		fqdn[len-1] = '.';
		p += p[0] + 1;
	}
	fqdn[len] = '\0';
	return fqdn;
}
