#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include "server.h"


#pragma comment(lib, "ws2_32")

#define PORT 5000


using namespace std;


int main() {

	ServerController server = ServerController();

	server.init(PORT);

	server.acceptClient();


	return 0;
}