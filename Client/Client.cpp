#include <iostream>
#include <windows.h>
#include <ws2tcpip.h>
#include <cstdlib>
#include <conio.h>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE 512
#define SERVER_PORT "27015"

SOCKET ClientSocket = INVALID_SOCKET;

const int maxX = 80;
const int maxY = 25;

int posX = 5;
int posY = 5;

void RemoveCursorBlink() {
	CONSOLE_CURSOR_INFO cursorInfo;
	cursorInfo.dwSize = 1;
	cursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

void RenderSmiley() {
	system("cls");

	COORD coord = { posX, posY };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);

	cout << ":)";
}

DWORD WINAPI SendCommand(void* data) {

	char cmd[2] = " ";

	RenderSmiley();

	while (true) {
		if (_kbhit()) {
			int ch = _getch();
			if (ch == 224 || ch == 0) ch = _getch();
			
			switch (ch) {
				case 72: // up
					cmd[0] = 'u';
					if (posY > 0) posY--;
					break;
				case 80: // down
					cmd[0] = 'd';
					if (posY < maxY - 1) posY++;
					break;
				case 77: // right
					cmd[0] = 'r';
					if (posX < maxX - 3) posX++;
					break;
				case 75: // left
					cmd[0] = 'l';
					if (posX > 0) posX--;
					break;
			}

			int sendResult = send(ClientSocket, cmd, 2, 0);
			if (sendResult == SOCKET_ERROR) {
				cout << "Failed to send data: " << WSAGetLastError() << endl;
				closesocket(ClientSocket);
				WSACleanup();
				exit(1);
			}

			RenderSmiley();
		}
	}

	return 0;
}

DWORD WINAPI ReceiveResponse(void* data) {
	while (true) {
		char buffer[BUF_SIZE];
		int bytesReceived = recv(ClientSocket, buffer, BUF_SIZE, 0);
		buffer[bytesReceived] = '\0';

		if (bytesReceived > 0) {
			cout << buffer << endl;
		}
		else if (bytesReceived == 0)
			cout << "Connection closed by server.\n";
		else
			cout << "recv failed: " << WSAGetLastError() << endl;
	}
	return 0;
}

int main() {
	setlocale(0, "");
	system("title Smiley Position Control");

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "WSAStartup failed.\n";
		return 1;
	}

	struct addrinfo *result = nullptr, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (getaddrinfo("localhost", SERVER_PORT, &hints, &result) != 0) {
		cout << "getaddrinfo failed.\n";
		WSACleanup();
		return 1;
	}

	for (auto ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
		ClientSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ClientSocket == INVALID_SOCKET) {
			cout << "socket failed with error: " << WSAGetLastError() << endl;
			WSACleanup();
			return 1;
		}

		if (connect(ClientSocket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
			closesocket(ClientSocket);
			ClientSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ClientSocket == INVALID_SOCKET) {
		cout << "Unable to connect to server!\n";
		WSACleanup();
		return 1;
	}

	RemoveCursorBlink();

	HANDLE hThreadSend = CreateThread(nullptr, 0, SendCommand,
