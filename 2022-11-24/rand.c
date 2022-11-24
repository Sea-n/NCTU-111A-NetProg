#include <stdio.h>
#include <stdlib.h>

int main() {
	static int oseed;
	static char buf[64];
	static int len;

	for (;;) {
		printf("seed:\n> ");
		scanf("%x", &oseed);
		srand(oseed);

		len  = snprintf(buf,     10, "%x", rand());
		len += snprintf(buf+len, 10, "%x", rand());
		len += snprintf(buf+len, 10, "%x", rand());
		len += snprintf(buf+len, 10, "%x", rand());
		printf("The secret is ``%s`` (from seed %x)\n", buf, oseed);

		rand();
		len  = snprintf(buf,     10, "%x", rand());
		len += snprintf(buf+len, 10, "%x", rand());
		len += snprintf(buf+len, 10, "%x", rand());
		len += snprintf(buf+len, 10, "%x", rand());
		printf("The secret is ``%s`` (from seed %x)\n", buf, oseed);
	}
}
