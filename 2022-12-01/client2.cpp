#include "generic.h"
#include <pthread.h>

#define CHUNK 130'000

int main(int argc, char *argv[]) {
	if (argc == 5) {
		char buf[CHUNK];
		memset(buf, 'A', CHUNK);
		execl(argv[0], argv[0], buf, NULL);
	}

	// Merge all files into one buffer
	char filename[256];
	int size;
	for (int k=0; k<1000; k++) {
		sprintf(filename, "/files/%06d", k);
		ifstream file(filename, ios::binary | ios::ate);
		size = file.tellg();
		sprintf(argv[1] + 7, "%06d", size);
		file.seekg(0, ios::beg);
		file.read(argv[1] + 14, size);
		sprintf(argv[1], "%06d", k);
		usleep(100);
		if (k == 0) usleep(1000);
	}
}
