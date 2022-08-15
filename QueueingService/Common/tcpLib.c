#include "tcpLib.h"

int initializeWindowsSockets()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return 0;
	}
	return 1;
}

SOCKET connectToSocket(struct sockaddr* address, int sockAddrLen)
{
	SOCKET connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//napravimo soket

	if (connect(connectSocket, (struct sockaddr*)address, sockAddrLen) == SOCKET_ERROR) //pozovemo connect
	{
		printf("Unable to connect to server.\n");
		closesocket(connectSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}

	unsigned long mode = 1;
	if (ioctlsocket(connectSocket, FIONBIO, &mode) == SOCKET_ERROR)//prebacimo u neblokirajuci rezim
	{
		printf("ioctlsocket failed with error: %ld\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}

	return connectSocket;
}

int sendData(SOCKET socket, char* data, int size)
{
	int sent = 0;
	
	FD_SET write;
	struct timeval timeVal;
	timeVal.tv_sec = 0;
	timeVal.tv_usec = 0;

	while (sent < size)
	{
		FD_ZERO(&write);
		FD_SET(socket, &write);
		int result = select(0, NULL, &write, NULL, &timeVal);
		if (result == SOCKET_ERROR)
		{
			printf("\nselect failed with error: %d\n", WSAGetLastError());
			return -1;
		}
		else if (result == 0)
		{
			Sleep(SLEEP_TIME);
			continue;
		}
		else if (FD_ISSET(socket, &write))
		{
			result = send(socket, data + sent, size - sent, 0);
			if (result == SOCKET_ERROR)
			{
				printf("\nsend failed with error: %d\n", WSAGetLastError());
				return -1;
			}
			else
			{
				sent += result;
			}
		}
	}

	return sent;
}

int receiveData(SOCKET socket, char* data, int dataSize, int tryOnce)
{
	int recvd_size = 0;
	FD_SET read;
	struct timeval timeVal;
	timeVal.tv_sec = 0;
	timeVal.tv_usec = 0;

	do
	{
		FD_ZERO(&read);
		FD_SET(socket, &read);
		int result = select(0, &read, NULL, NULL, &timeVal);
		if (result == SOCKET_ERROR)
		{
			printf("\nselect failed with error: %d\n", WSAGetLastError());
			return -1;
		}
		else if (result == 0)
		{
			Sleep(100);

			if (tryOnce) return -2;

			continue;
		}
		else if (FD_ISSET(socket, &read))
		{
			result = recv(socket, data + recvd_size, dataSize - recvd_size, 0);
			if (result == SOCKET_ERROR)
			{
				printf("\nrecv failed with error: %d\n", WSAGetLastError());
				return -1;
			}
			else
			{
				recvd_size += result;
			}
		}

	} while (recvd_size != 256);

	return recvd_size;
}
