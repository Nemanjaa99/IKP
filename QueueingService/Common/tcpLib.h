#ifndef TCPLIB_H
#define TCPLIB_H

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "Ws2_32.lib")

#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include <winsock2.h>

#define IP_ADDRESS "127.0.0.1"
#define QUEUEINGSERVICE_PORT1 16000
#define QUEUEINGSERVICE_PORT2 16001

#define SLEEP_TIME 100

int initializeWindowsSockets();

SOCKET connectToSocket(struct sockaddr* address, int sockAddrLen);

int sendData(SOCKET socket, char* data, int size);

int receiveData(SOCKET socket, char* data, int dataSize, int tryOnce);

#endif


