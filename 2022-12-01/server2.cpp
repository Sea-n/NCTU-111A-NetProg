#include "generic.h"

int main(int argc, char *argv[]) {
	char filename[128], buffer[132000];
	int pid = getpid() + 1;

	usleep(1000);
	printf("Pid: %d\n", pid);
	sprintf(filename, "/proc/%d/cmdline", pid);
	ifstream file(filename, ios::binary | ios::in);
	for (;;) {
		file.seekg(0, ios::beg);
		file.read(buffer, 130'000);
		if (buffer[13] == filename[12]) continue;
		sprintf(filename, "/files/%s", buffer+8);
		ofstream ofile(filename, ios::binary | ios::out);
		ofile.write(buffer+22, atoi(buffer+15));
	}
}
