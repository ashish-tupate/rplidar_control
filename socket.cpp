// Socket for sending data locally to python

// Link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

SOCKET open_sock(int port)
{
	// Function opens socket
	WSADATA WSAData;
	SOCKET server;
	SOCKADDR_IN addr;

	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
	{
		printf("Winsock error - Winsock initialization failed\r\n");
		WSACleanup();
	}

	server = socket(AF_INET, SOCK_STREAM, 0);
	if (server == INVALID_SOCKET)
	{
		printf("Socket creation failed.\r\n");
		WSACleanup();
	}

	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if (connect(server, (SOCKADDR*)(&addr), sizeof(addr)) != 0)
	{
		printf("Failed to establish connection with server\r\n");
		WSACleanup();
	}
	else {printf("Connected to server!!!");}

	return server;
}

void send_data(SOCKET server, unsigned short distance, float angle, char quality)
{
	// Function sends data via socket
	// Just need to work out how to pack the data into the buffer, then I'm all good!
	unsigned int angle_to_send;
	angle_to_send = static_cast<unsigned int>(angle * 1000000);  // Scale float to send it as an int

	send(server, (char *)&distance, sizeof(distance), 0);
	send(server, (char *)&angle_to_send, sizeof(angle_to_send), 0);
	send(server, &quality, sizeof(quality), 0);
	return;
}

