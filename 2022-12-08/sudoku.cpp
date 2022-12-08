#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>

#define FAILED "111222333444555666777888999111222333444555666777888999111222333444555666777888999"

using namespace std;

string snd(const char *txt);
string solve(string board);
bool check(string board);

int sock;

int main() {
	struct sockaddr_un addr;
	string board, ans;
	char buf[420];

	// Socket init
	sock = socket(AF_LOCAL, SOCK_STREAM, 0);
	bzero(&addr, sizeof(addr));
	addr.sun_family = AF_LOCAL;
	strcpy(addr.sun_path, "/sudoku.sock");
	connect(sock, (struct sockaddr *) &addr, sizeof(addr));

	// Solve
	board = snd("S").substr(4);
	printf("Board: %s\n", board.c_str());
	ans = solve(board);
	printf("Answer: %s\n", ans.c_str());

	// Send and check
	for (int i=0; i<9; i++)
		for (int j=0; j<9; j++)
			if (ans[i*9+j] != board[i*9+j]) {
				sprintf(buf, "V %d %d %c", i, j, ans[i*9+j]);
				snd(buf);
			}
	snd("C");
}

string snd(const char *txt) {
	char recv_buf[420];
	send(sock, txt, strlen(txt), 0);
	int n = read(sock, recv_buf, 420);
	recv_buf[n] = '\0';
	return recv_buf;
}

string solve(string board) {
	if (!check(board)) return FAILED;
	for (int i=0; i<81; i++)
		if (board[i] == '.') {
			for (int k=1; k<=9; k++) {
				board[i] = '0' + k;
				string ans = solve(board);
				if (check(ans))
					return ans;
			}
			return FAILED;
		}
	return board; // No empty slot -> answer
}

bool check(string board) {
	int flag[3][9][9] = {{{}}};
	for (int i=0; i<9; i++)
		for (int j=0; j<9; j++) {
			if (board[i*9 + j] == '.') continue;

			if (flag[0][i][ board[i*9+j] - '1' ]) return false;
			if (flag[1][j][ board[i*9+j] - '1' ]) return false;
			if (flag[2][i/3*3 + j/3][ board[i*9+j] - '1' ]) return false;

			flag[0][i][ board[i*9+j] - '1' ] = 1;
			flag[1][j][ board[i*9+j] - '1' ] = 1;
			flag[2][i/3*3 + j/3][ board[i*9+j] - '1' ] = 1;
		}
	return true;  // No rule breaks -> check pass
}
