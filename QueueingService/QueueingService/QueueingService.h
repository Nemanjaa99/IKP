/*#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib,"Ws2_32.lib")

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>*/

#include "../Common/Queue.h"
#include "../Common/tcpLib.h"

#define MAX_PROCESSES 10

#pragma pack(1)
typedef struct thread1_param
{
	SOCKET* processesSockets;
	int processNum;
	SOCKET queueingServiceSocket;
	QUEUE* queue;
	QUEUEARRAY* queueArray;
}THREAD1_PARAM;

DWORD WINAPI receiveQueingServiceAndProcessData(LPVOID param);//pokazivac na bilo sta

#pragma pack(1)
typedef struct thread2_param
{
	SOCKET queueingServiceSocket;
	QUEUE* queue;
}THREAD2_PARAM;

DWORD WINAPI sendDataToQueueingService(LPVOID param);

#pragma pack(1)
typedef struct thread3_param
{
	QUEUEARRAY* queueArray;
}THREAD3_PARAM;

DWORD WINAPI sendDataToProcesses(LPVOID param);


