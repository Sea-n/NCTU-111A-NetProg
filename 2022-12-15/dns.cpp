#include "dns.h"

string fqdn2name(const char *fqdn);
string name2fqdn(const char *name);

int main(int argc, char *argv[]) {
	struct sockaddr_in sin, csin;
	socklen_t sinlen = sizeof(sin);
	char rcv_buf[MTU], snd_buf[MTU];
	char fwd_ns[64], qname_raw[512];
	char line1[512], line2[512];
	DNS *rcv = (DNS *) rcv_buf;
	DNS *snd = (DNS *) snd_buf;
	char *p, *ptr1, *ptr2;
	vector<ZONE> zones;
	string dn, fn;
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
			rr.rdlen = 0;

			rr.name = strtok_r(line2, ",", &ptr2);
			rr.fqdn = (rr.name == "@") ? dn : (rr.name + "." + dn);
			rr.full_name = fqdn2name(rr.fqdn.c_str());
			rr.ttl = atoi(strtok_r(nullptr, ",", &ptr2));

			p = strtok_r(nullptr, ",", &ptr2);  // Class
			if (strcmp(p, "IN") == 0) rr.clazz = 1;
			if (strcmp(p, "CS") == 0) rr.clazz = 2;
			if (strcmp(p, "CH") == 0) rr.clazz = 3;
			if (strcmp(p, "HS") == 0) rr.clazz = 4;

			p = strtok_r(nullptr, ",", &ptr2);  // Type
			if (strcmp(p, "A") == 0) {
				rr.type = TYPE_A;
				unsigned int *ui = (unsigned int *) rr.rdata;
				*ui = inet_addr(strtok_r(nullptr, "", &ptr2));  // Address
				rr.rdlen += 4;
			}

			if (strcmp(p, "NS") == 0) {
				rr.type = TYPE_NS;
				rr.need_a_rr = strtok_r(nullptr, "\r\n ", &ptr2);
				strcpy(rr.rdata, fqdn2name(rr.need_a_rr.c_str()).c_str());
				rr.rdlen += strlen(rr.rdata);  // NSDNAME
				rr.rdata[rr.rdlen++] = '\0';
			}

			if (strcmp(p, "CNAME") == 0) {
				rr.type = TYPE_CNAME;
			}

			if (strcmp(p, "SOA") == 0) {
				rr.type = TYPE_SOA;
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

			if (strcmp(p, "MX") == 0) {
				rr.type = TYPE_MX;
				int pref = atoi(strtok_r(nullptr, " ", &ptr2));
				rr.rdata[0] = pref >> 8; rr.rdata[1] = pref & 0xFF;
				rr.rdlen += 2;  // Preference

				rr.need_a_rr = strtok_r(nullptr, "\r\n ", &ptr2);
				strcpy(rr.rdata+2, fqdn2name(rr.need_a_rr.c_str()).c_str());
				rr.rdlen += strlen(rr.rdata+2);  // Exchange
				rr.rdata[rr.rdlen++] = '\0';
			}

			if (strcmp(p, "TXT") == 0) {
				rr.type = TYPE_TXT;
			}

			if (strcmp(p, "AAAA") == 0) {
				rr.type = TYPE_AAAA;
			}

			// Construct packet buffer
			strcpy(rr.buf, rr.full_name.c_str());
			rr.buf_len = strlen(rr.buf);
			rr.buf[rr.buf_len++] = '\0';
			rr.buf[rr.buf_len++] = 0; rr.buf[rr.buf_len++] = rr.type;
			rr.buf[rr.buf_len++] = 0; rr.buf[rr.buf_len++] = rr.clazz;

			int *rr_ttl = (int *) &rr.buf[rr.buf_len];
			*rr_ttl = __builtin_bswap32(rr.ttl); rr.buf_len += 4;

			rr.buf[rr.buf_len++] = rr.rdlen >> 8;
			rr.buf[rr.buf_len++] = rr.rdlen & 0xFF;
			memcpy(rr.buf+rr.buf_len, rr.rdata, rr.rdlen);
			rr.buf_len += rr.rdlen;

			rrs.push_back(rr);
		}
		zones.push_back({dn, rrs});
	}

	for (;;) {  // Main loop
		recvfrom(sock, rcv_buf, MTU, 0, (struct sockaddr*) &csin, &sinlen);
		p = rcv_buf + sizeof(DNS);
		strcpy(qname_raw, p);
		string qname = name2fqdn(qname_raw);
		p += qname.size();
		int qtype = p[2];  // Only support up to 255
		int qclass = p[4];

		int snd_len = sizeof(DNS);
		vector<string> need_a_rr;
		memset(snd_buf, 0, MTU);

		for (ZONE zone : zones) {
			if (qname != zone.F && !qname.ends_with('.' + zone.F))
				continue;

			// Question Section
			strcpy(snd_buf+snd_len, rcv_buf + sizeof(DNS));
			snd_len += strlen(snd_buf+snd_len);
			snd_buf[snd_len++] = '\0';
			snd_buf[snd_len++] = 0; snd_buf[snd_len++] = qtype;
			snd_buf[snd_len++] = 0; snd_buf[snd_len++] = qclass;
			snd->qdcount += 1;

			// Answer Section
			for (RR rr : zone.S) {
				if (qname == zone.F) {
					if (rr.name != "@") continue;
				} else {
					if (qname != rr.name + '.' + zone.F) continue;
				}
				if (qtype != rr.type) continue;
				if (qclass != rr.clazz) continue;

				if (rr.need_a_rr != "") need_a_rr.push_back(rr.need_a_rr);

				memcpy(snd_buf+snd_len, rr.buf, rr.buf_len);
				snd_len += rr.buf_len;
				snd->ancount += 1;
			}

			// Authority Section
			for (RR rr : zone.S) {
				if (rr.name != "@") continue;

				if (rr.type == qtype) continue;  // Alreay in answer
				if (snd->ancount) {
					if (rr.type != TYPE_NS) continue;
				} else {
					if (rr.type != TYPE_SOA) continue;
				}

				memcpy(snd_buf+snd_len, rr.buf, rr.buf_len);
				snd_len += rr.buf_len;
				snd->nscount += 1;
			}

			// Additional Section
			for (RR rr : zone.S) {
				if (rr.type != TYPE_A) continue;
				if (find(need_a_rr.begin(), need_a_rr.end(), rr.fqdn) == need_a_rr.end()) continue;

				memcpy(snd_buf+snd_len, rr.buf, rr.buf_len);
				snd_len += rr.buf_len;
				snd->arcount += 1;
			}

			snd->id = rcv->id;
			snd->qr = 1;
			snd->aa = 1;
			snd->ra = 1;
			goto response;
		}  // End of for zones

		// Forward

response:
		sendto(sock, snd_buf, snd_len, 0, (struct sockaddr*) &csin, sinlen);
	}  // End of main loop
}  // End of main()

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
