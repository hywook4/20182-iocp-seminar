#pragma once
#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <WinSock2.h>

#define BUFSIZE 1024

using namespace std;

typedef struct {
	OVERLAPPED overlapped;
	char buffer[BUFSIZE];
	WSABUF wsaBuf;
} IO_DATA, *LP_IO_DATA;

typedef struct {
	SOCKET clSock;
	SOCKADDR_IN clAddr;
} HANDLE_DATA, *LP_HANDLE_DATA;

class ServerController {
private:
	WSADATA wsaData;

	SOCKET listenSock;
	SOCKADDR_IN listenAddr;

	LP_IO_DATA ioData;
	LP_HANDLE_DATA handleData;

	HANDLE completionPort;
	SYSTEM_INFO systemInfo;

	int size;

public:

	ServerController();

	bool init(const int port);
	bool acceptClient();

	unsigned int __stdcall completionThread(LPVOID iocp);

	static UINT WINAPI completionFunc(LPVOID object) {
		ServerController *control = (ServerController*)object;
		control->completionThread(control->completionPort);
		return 0;
	}

};










#endif