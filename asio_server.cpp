// asio_server.cpp : Defines the entry point for the console application.
//

#include "Service.h"
#include "TcpServer.h"
int main(int argc, char* argv[])
{
	auto tcpServer = nicehero::TcpServer("192.168.1.43", 8888);
	nicehero::StartService();
	return 0;
}

