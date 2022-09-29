#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <unistd.h>

using namespace std;

struct Header {
	char magic[4];
	int offset_string;
	int offset_content;
	int file_count;
};

struct FILE_E {
	int offset_string;
	int filesize_be;
	int offset_content;
	char checksum[8];
};

char *buf = new char[1024 * 1024 * 1024];

int main(int argc, char *argv[]) {
	Header header;
	vector<FILE_E> files;
	char dirname[640];
	char filename[640];

	// Check arguments
	if (argc != 3) {
		cerr << "Usage: " << argv[0] << " <src.pak> <dst>\n";
		return 1;
	}
	strcpy(dirname, argv[2]);
	int dir_len = strlen(argv[2]) + 1;
	dirname[dir_len - 1] = '/';
	dirname[dir_len] = '\0';

	// Read file into vector
	ifstream pak(argv[1], ios::binary);
	pak.read((char*)&header, sizeof(Header));

	// Check magic number
	if (strncmp(header.magic, "PAKO", 4)) {
		cerr << "Magic number incorrect.\n";
		return 1;
	}
	cout << "File count: " << header.file_count << '\n';
	cout << "Offset: " << header.offset_content << ' ' << header.offset_string << '\n';

	for (int k=0; k<header.file_count; k++) {
		FILE_E file;
		pak.read((char*)&file, sizeof(FILE_E));
		file.filesize_be = __builtin_bswap32(file.filesize_be);

		files.push_back(file);
	}

	for (FILE_E file : files) {
		strcpy(filename, dirname);
		pak.seekg(header.offset_string + file.offset_string);
		pak >> (filename + dir_len);
		cout << "Filename: " << (filename + dir_len) << "  (" << file.filesize_be << " bytes)\n";
		ofstream out(filename, ios::binary);

		// Extract file
		pak.seekg(header.offset_content + file.offset_content);
		pak.read(buf, file.filesize_be);
		out.write(buf, file.filesize_be);

		// Check checksum
		bool corrupted = false;
		char chk[8] = {0, 0, 0, 0, 0, 0, 0, 0};
		for (int i=0; i<file.filesize_be; i++)
			chk[i%8] ^= buf[i];
		for (int i=0; i<8; i++)
			if (chk[7-i] != file.checksum[i])
				corrupted = true;

		// Clean up
		out.close();

		if (corrupted) {
			cerr << "Warning: File " << filename << " is corrupted.\n";
			unlink(filename);
		}
	}
	return 0;
}
