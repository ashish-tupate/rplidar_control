#pragma once

#include <WinSock2.h>

using namespace std;

SOCKET open_sock(int port);
void send_data(SOCKET server, unsigned short distance, float angle, char quality);