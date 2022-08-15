#ifndef QUEUE_H
#define QUEUE_H

#include "tcpLib.h"

/*#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "Ws2_32.lib")*/

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <winsock2.h>


#define ARRAY_SIZE 10
#define PROCESS_STRING_SIZE 32
#define BUFFER_SIZE 256
#define DATA_SIZE 220 // BUFFER_SIZE - PROCESS_STRING_SIZE - 4

enum MESSAGE_TYPE { QUEUEING_SERVICE_DATA = 0, PROCESS_DATA = 1 };

#pragma pack(1)
typedef struct Message_struct //podaci koji se salju izmedju QS
{
	enum MESSAGE_TYPE type;
	char processName[PROCESS_STRING_SIZE];//za koga je poruka, naziv procesa, naziv reda
	char data[DATA_SIZE];//poruka
}MESSAGE;

#pragma pack(1)
typedef struct QueueNode
{
	MESSAGE message;
	struct QueueNode* next;
	struct QueueNode* prev;
}NODE;

#pragma pack(1)
typedef struct Queue_structure //uvek skidamo element sa taila a dodajemo na frony
{
	CRITICAL_SECTION cs;
	NODE* front;//ako pokazuje na null nema nista u queue
	NODE* tail;
}QUEUE;

#pragma pack(1)
typedef struct QueueArray_structure
{
	int size;//koliko elemenata imamo, br razlicitih procesa
	CRITICAL_SECTION cs; //tred sejf
	char processNames[ARRAY_SIZE][PROCESS_STRING_SIZE];//matrica svih mogucih proecsa koji postoje
	SOCKET processSockets[ARRAY_SIZE];// niz soketa , moramo da znamo koji soketi koji komunic s tim procesima
	QUEUE processQueues[ARRAY_SIZE];//za svaki proces imamo kju
}QUEUEARRAY;

void initializeQueueArray(QUEUEARRAY* queueArray);

int getQueueArraySize(QUEUEARRAY* queueArray);

void insertInQueueArray(QUEUEARRAY* queueArray, MESSAGE* message);

int addProcessToQueueArray(QUEUEARRAY* queueArray, char* processName, SOCKET s);

void initializeQueue(QUEUE* queue);

int pop(QUEUE* queue, MESSAGE* message); // make sure to release memory

void push(QUEUE* queue, MESSAGE* message); // make sure to allocate and copy data

#endif