#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <list>
#include "server.h"
#include <map>
#include <string>

using namespace std;


void Error(const char* errorcode) {

	printf("%s\n", errorcode);

	exit(1);
}

ServerController::ServerController(){

	listenSock = NULL;
	GetSystemInfo(&systemInfo);

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		Error("WSAStartup() error\n");

}

bool ServerController::init(const int port) {

	completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (completionPort == NULL)
		Error("CreateIoCompletionPort() error\n");

	//스레드풀 생성
	for (int i = 0; i < (systemInfo.dwNumberOfProcessors) - 2; i++)
		_beginthreadex(NULL, 0, completionFunc, (LPVOID)this, 0, NULL);

	listenSock = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (listenSock == INVALID_SOCKET)
		Error("WSASocket() error\n");

	listenAddr.sin_family = AF_INET;
	listenAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	listenAddr.sin_port = htons((short)port);

	if (_WINSOCK2API_::bind(listenSock, (SOCKADDR*)&listenAddr, sizeof(listenAddr)) == SOCKET_ERROR)
		Error("bind() error\n");

	if (listen(listenSock, SOMAXCONN) == -1)
		Error("listen() error\n");


	return true;
}


unsigned int __stdcall ServerController::completionThread(LPVOID iocp) {
	HANDLE completionPort = (HANDLE)iocp;

	int transferredBytes;
	int sentBytes;

	LP_HANDLE_DATA handleData;
	LP_IO_DATA ioData;

	int recvBytes;
	bool recvFlag;

	DWORD flag = 0;


	WSABUF* wsaBuf = new WSABUF;


	while (1) {

		
		int offset = 0;

		//gqcs 
		//printf("waiting for recv\n");
		recvFlag = GetQueuedCompletionStatus(completionPort, (LPDWORD)&transferredBytes, (PULONG_PTR)&handleData, (LPOVERLAPPED*)&ioData, INFINITE);
		printf("received something\n");
		//정상 
		if (recvFlag) {
			//client closed
			if (transferredBytes == 0) {
				printf("bye client\n");
				closesocket(handleData->clSock);

			}
			//received data
			else {
				ioData->buffer[transferredBytes] = '\0';
				
				printf("Received message : %s (%d bytes)\n", ioData->buffer, transferredBytes);

				ioData->wsaBuf.len = transferredBytes;
				
				if (WSASend(handleData->clSock, &(ioData->wsaBuf), 1, (LPDWORD)&sentBytes, 0, NULL, NULL) == SOCKET_ERROR) {
					if (WSAGetLastError() != WSA_IO_PENDING)
						printf("Error - failed WSASend()\n");
				}
				

				memset(&(ioData->overlapped), 0, sizeof(OVERLAPPED));
				ioData->wsaBuf.len = BUFSIZE;
				ioData->wsaBuf.buf = ioData->buffer;

				WSARecv(handleData->clSock, &(ioData->wsaBuf), 1, (LPDWORD)&recvBytes, (LPDWORD)&flag, &(ioData->overlapped), NULL);

				flag = 0;
				recvFlag = 0;
			}
			
		}

		//비정상 
		else {
			//데이터 크기가 0인 경우 클라이언트와 접속이 끊긴 것
			if (transferredBytes == 0) {
				printf("Bad disconnection\n");
			}
			printf("Something went bad\n");
		}

	}
	return 0;
}

bool ServerController::acceptClient() {
	printf("accept start\n");

	int flag;
	int recvBytes;
	int sentBytes;
	


	while (1) {
		SOCKET clientSock;
		SOCKADDR_IN clientAddr;


		int addrSize = sizeof(clientAddr);

		//여기서는 accept들어올때까지 막힘!!
		if ((clientSock = accept(listenSock, (SOCKADDR*)&clientAddr, &addrSize)) != -1) {
			printf("Connected to Client\n");
			printf("sin_port : %d\n", clientAddr.sin_port);
		}
		else
			Error("accept() error\n");

		handleData = (LP_HANDLE_DATA)malloc(sizeof(HANDLE_DATA));
		handleData->clSock = clientSock;
		memcpy(&(handleData->clAddr), &clientAddr, addrSize);

		CreateIoCompletionPort((HANDLE)clientSock, completionPort, (DWORD)handleData, 0);

		
		ioData = (LP_IO_DATA)malloc(sizeof(IO_DATA));
		memset(&(ioData->overlapped), 0, sizeof(OVERLAPPED));
		ioData->wsaBuf.len = BUFSIZE;
		ioData->wsaBuf.buf = ioData->buffer;

		flag = 0;

		if (WSARecv(handleData->clSock, &(ioData->wsaBuf), 1, (LPDWORD)&recvBytes, (LPDWORD)&flag, &(ioData->overlapped), NULL) == SOCKET_ERROR) {
			int error = WSAGetLastError();
			if (error != WSA_IO_PENDING)
				printf("WSARecv() error %d\n", error);
		}

	}
	return true;
}
