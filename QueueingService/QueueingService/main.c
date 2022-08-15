#include "QueueingService.h"

int insertProcesses(char* processes);

void runFirstQueueingService();

void runSecondQueueingService();


int main(int argc, char** argv)
{
	printf("Queueing servis 1 ili 2: ");
	int pc = 0;
	do {
		scanf("%d", &pc);
		if (pc != 1 && pc != 2)
			printf("Mora biti 1 or 2...\n");
	}while (pc != 1 && pc != 2);

	getchar();
	
	
	// Prvi Queueing servis
	if (pc == 1)
	{
		runFirstQueueingService();
	}
	// Drugi Queueing servis
	else if (pc == 2)
	{
		runSecondQueueingService();
	}

	return 0;
}

int insertProcesses(char** processes)
{
	printf("Unesite nazive procesa. Unesite 'Kraj' za kraj unosa.\n");
	
	char temp[PROCESS_STRING_SIZE];
	int counter = 0;
	int dataLen = 0;
	do
	{
		gets(temp);

		if (strcmp(temp, "Kraj") == 0)
		{
			break;
		}

		// Unosenje podataka, pokazivac na 1 bajt
		memcpy((char*)processes + counter*PROCESS_STRING_SIZE, temp, strlen(temp));//ako nije kraj unosimo podatke u matricu

		counter++;

	} while (strcmp(temp, "Kraj") && counter < MAX_PROCESSES);

	return counter;
}

void runFirstQueueingService()
{
	if (!initializeWindowsSockets())
		return 1;

	printf("Pokrenuti prvi Queueing servis!\n");

	// Otvaranje konekcije
	SOCKET listenSocket = INVALID_SOCKET; //imamo neku adresnu strukturu
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET; // popunimo podatke za strukturu
	serverAddress.sin_addr.s_addr = inet_addr(IP_ADDRESS);
	serverAddress.sin_port = htons(QUEUEINGSERVICE_PORT1);
	int sockAddrLen = sizeof(struct sockaddr_in);

	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //od lisn soketa pravimo soket
	if (listenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		closesocket(listenSocket);
		return 1;
	}

	// Bind
	int iResult = bind(listenSocket, (struct sockaddr*)&serverAddress, sockAddrLen);//bajndujemo ga
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	// Listen
	iResult = listen(listenSocket, SOMAXCONN); //postavljamo u stanje slusanja
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	unsigned long mode = 1;
	struct sockaddr_in queueingServiceAddr;

	SOCKET queueingServiceSocket = accept(listenSocket, (struct sockaddr*)&queueingServiceAddr, &sockAddrLen);//zatim radimo accept, i dalje je u blokirajucem 
	if (queueingServiceSocket == INVALID_SOCKET)//za to vreme drugi kjuing servis
	{
		printf("Greska prilikom povezivanja.\n");
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	printf("Veza sa drugim Queueing servisom ostvarena!\n");

	// Unos imena procesa
	char processes[MAX_PROCESSES][PROCESS_STRING_SIZE];//niz stringova
	memset(processes, 0, MAX_PROCESSES * PROCESS_STRING_SIZE);
	int processNum = insertProcesses(processes);

	// Inicijalizacija struktura podataka
	QUEUE queue;
	initializeQueue(&queue);
	QUEUEARRAY queueArray;
	initializeQueueArray(&queueArray);

	// Informacije o procesu
	STARTUPINFO si[MAX_PROCESSES];
	PROCESS_INFORMATION pi[MAX_PROCESSES];

	// Socketi za procese
	int connectedNum = 0;//ako pokrenemo 3 porcesa == 3 soketa
	SOCKET processSockets[MAX_PROCESSES];
	for (int i = 0; i < MAX_PROCESSES; i++)
		processSockets[i] = INVALID_SOCKET;

	// Pokretanje procesa i prihvatanje njihovih zahteva za konekciju
	for (int i = 0; i < processNum; i++) //insert proces nam vraca broj procesa
	{
		ZeroMemory(&(si[i]), sizeof(STARTUPINFOA));
		si[i].cb = sizeof(STARTUPINFOA);
		ZeroMemory(&(pi[i]), sizeof(PROCESS_INFORMATION));
		//saljemo argumente procesima, agrumente main funkcije
		char* processArguments = (char*)malloc(PROCESS_STRING_SIZE + 2 * sizeof(char));//zauzimamo memoriju
		char* processName = (char*)processes + i * PROCESS_STRING_SIZE;//pokazivac na i-ti proces
		memcpy(processArguments, "1\x20", 2 * sizeof(char));//kopiramo dva karaktera
		memcpy(processArguments + 2 * sizeof(char), processName, PROCESS_STRING_SIZE);//adresnom aritmetikom preskocimo dva karaktera i upisemo naziv njegovog reda
		
		if (!CreateProcessA("../Debug/Process.exe", processArguments, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &(si[i]), &(pi[i])))//relativna putanja procesa
		{
			printf("CreateProcess failed (%d).\n", GetLastError());
			return;
		}

		processSockets[connectedNum] = accept(listenSocket, NULL, NULL);//prihvatamo zahtev za konekciju

		if (processSockets[connectedNum] == INVALID_SOCKET)
		{
			printf("accept failed with error: %d\n", WSAGetLastError());
			return -1;
		}

		iResult = ioctlsocket(processSockets[connectedNum], FIONBIO, &mode);//prebacimo u neblokirajuci rezim
		if (iResult != NO_ERROR)
		{
			printf("ioctlsocket failed with error: %ld\n", iResult);
			return -1;
		}
		//dodajemo, imamo vec soket ii naziv procesa koji je pokrenut
		addProcessToQueueArray(&queueArray, processName, processSockets[connectedNum]);

		connectedNum++;
	}

	// Pokrenuti thread za prijem poruka od procesa i Queueing servisa
	THREAD1_PARAM parameter1;
	parameter1.queueingServiceSocket = queueingServiceSocket;
	parameter1.processesSockets = processSockets;
	parameter1.processNum = processNum;
	parameter1.queue = &queue;
	parameter1.queueArray = &queueArray;
	int acceptAndRecvDataId = 0;
	HANDLE acceptAndRecvDataHandle = CreateThread(NULL, 0, &receiveQueingServiceAndProcessData, &parameter1, 0, &acceptAndRecvDataId);

	// Pokrenuti thread za slanje Queueing servisu
	THREAD2_PARAM parameter2;
	parameter2.queue = &queue;
	parameter2.queueingServiceSocket = queueingServiceSocket;
	int sendDataToQueueingServiceId = 0;
	HANDLE sendDataToQueueingServiceHandle = CreateThread(NULL, 0, &sendDataToQueueingService, &parameter2, 0, &sendDataToQueueingServiceId);

	// Pokrenuti thread za slanje poruka procesima
	THREAD3_PARAM parameter3;
	parameter3.queueArray = &queueArray;
	int sendDataToProcessesId = 0;
	HANDLE sendDataToProcessesHandle = CreateThread(NULL, 0, &sendDataToProcesses, &parameter3, 0, &sendDataToProcessesId);

	printf("Pritiskom na taster zatvoriti program.\n");
	getchar();

	// Zatvoriti sve resurse


	/*WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);*/
}

void runSecondQueueingService()
{
	if (!initializeWindowsSockets())
		return 1;

	printf("Pokrenuti drugi Queueing servis!\n");

	// Konektovanje na prvi Queueing servis
	SOCKET connectSocket = INVALID_SOCKET;
	struct sockaddr_in queueingServiceAddress; // popunjava iste podatke koji su vezani za QS1
	queueingServiceAddress.sin_family = AF_INET;
	queueingServiceAddress.sin_addr.s_addr = inet_addr(IP_ADDRESS);
	queueingServiceAddress.sin_port = htons(QUEUEINGSERVICE_PORT1);
	int sockAddrLen = sizeof(struct sockaddr);

	connectSocket = connectToSocket((struct sockaddr*)&queueingServiceAddress, sockAddrLen);//poziva funkciju da se konektuje na QS1
	if (connectSocket == INVALID_SOCKET)
	{
		printf("Greska prilikom povezivanja.\n");
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	// Otvoriti listen socket
	SOCKET listenSocket = INVALID_SOCKET;
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(IP_ADDRESS);
	serverAddress.sin_port = htons(QUEUEINGSERVICE_PORT2);
	sockAddrLen = sizeof(struct sockaddr_in);

	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		closesocket(listenSocket);
		return 1;
	}

	// Bind
	int iResult = bind(listenSocket, (struct sockaddr*)&serverAddress, sockAddrLen);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	// Listen
	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	printf("Konekcija sa prvim Queueing servisom ostvarena!\n");

	// Unos imena procesa
	char processes[MAX_PROCESSES][PROCESS_STRING_SIZE];
	memset(processes, 0, MAX_PROCESSES * PROCESS_STRING_SIZE);
	int processNum = insertProcesses(processes);

	// Inicijalizacija struktura podataka
	QUEUE queue;
	initializeQueue(&queue);
	QUEUEARRAY queueArray;
	initializeQueueArray(&queueArray);

	// Informacije o procesu
	STARTUPINFO si[MAX_PROCESSES];
	PROCESS_INFORMATION pi[MAX_PROCESSES];

	// Socketi za procese
	int connectedNum = 0;
	unsigned int mode = 1;
	SOCKET processSockets[MAX_PROCESSES];
	for (int i = 0; i < MAX_PROCESSES; i++)
		processSockets[i] = INVALID_SOCKET;

	// Pokretanje procesa i prihvatanje njihovih zahteva za konekciju
	for (int i = 0; i < processNum; i++)
	{
		ZeroMemory(&(si[i]), sizeof(STARTUPINFOA));
		si[i].cb = sizeof(STARTUPINFOA);
		ZeroMemory(&(pi[i]), sizeof(PROCESS_INFORMATION));

		char* processArguments = (char*)malloc(PROCESS_STRING_SIZE + 2 * sizeof(char));
		char* processName = (char*)processes + i * PROCESS_STRING_SIZE;
		memcpy(processArguments, "2\x20", 2 * sizeof(char));
		memcpy(processArguments + 2 * sizeof(char), processName, PROCESS_STRING_SIZE);

		if (!CreateProcessA("../Debug/Process.exe", processArguments, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &(si[i]), &(pi[i])))
		{
			printf("CreateProcess failed (%d).\n", GetLastError());
			return;
		}

		processSockets[connectedNum] = accept(listenSocket, NULL, NULL);

		if (processSockets[connectedNum] == INVALID_SOCKET)
		{
			printf("accept failed with error: %d\n", WSAGetLastError());
			return -1;
		}

		iResult = ioctlsocket(processSockets[connectedNum], FIONBIO, &mode);
		if (iResult != NO_ERROR)
		{
			printf("ioctlsocket failed with error: %ld\n", iResult);
			return -1;
		}

		addProcessToQueueArray(&queueArray, processName, processSockets[connectedNum]);

		connectedNum++;
	}

	// Pokrenuti thread za prijem poruka od procesa i Queueing servisa
	THREAD1_PARAM parameter1;
	parameter1.queueingServiceSocket = connectSocket;//soket koji sluzi za komunikacjiu sa drugim servisom
	parameter1.processesSockets = processSockets;//jedan koji sluzi za kom sa procesima
	parameter1.processNum = processNum;
	parameter1.queue = &queue;
	parameter1.queueArray = &queueArray;
	int acceptAndRecvDataId = 0;
	HANDLE acceptAndRecvDataHandle = CreateThread(NULL, 0, &receiveQueingServiceAndProcessData, &parameter1, 0, &acceptAndRecvDataId);//pokrecemo ovaj tred

	// Pokrenuti thread za slanje Queueing servisu
	THREAD2_PARAM parameter2;//ako saljemo poruku za kjuing servis samo nam treba soket preko koga komunicira ka drugom
	parameter2.queue = &queue;
	parameter2.queueingServiceSocket = connectSocket;
	int sendDataToQueueingServiceId = 0;
	HANDLE sendDataToQueueingServiceHandle = CreateThread(NULL, 0, &sendDataToQueueingService, &parameter2, 0, &sendDataToQueueingServiceId);//pokrecemo tred

	// Pokrenuti thread za slanje poruka procesima
	THREAD3_PARAM parameter3;
	parameter3.queueArray = &queueArray;
	int sendDataToProcessesId = 0;
	HANDLE sendDataToProcessesHandle = CreateThread(NULL, 0, &sendDataToProcesses, &parameter3, 0, &sendDataToProcessesId);

	printf("Pritiskom na taster zatvoriti program.\n");
	getchar();
}
