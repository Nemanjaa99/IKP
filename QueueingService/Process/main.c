#include <stdio.h>
#include "../Common/Queue.h"

int main(int argc, char** argv)
{
	if (!initializeWindowsSockets())
		return 1;

	if (argc < 2)//da li je dobar broj arg
	{
		printf("Neophodni redni broj QueueingService-a i ime processa\n");
		return 0;
	}

	unsigned short portNumber;
	if (strcmp(argv[0], "1") == 0)//proveramo da li je 1 ili 2 Qs, DA ZNAMO NA KOJI PORT DA SE NAKACIMO
	{
		portNumber = QUEUEINGSERVICE_PORT1;
	}
	else if (strcmp(argv[0], "2") == 0)
	{
		portNumber = QUEUEINGSERVICE_PORT2;
	}
	else
	{
		printf("Red %s ne postoji.\n", argv[0]);
		return;
	}

	printf("Pokrenut proces: %s\n", argv[1]);

	// Konektovanje na Queueing servis, klasican
	SOCKET connectSocket = INVALID_SOCKET;
	struct sockaddr_in queueingServiceAddress;
	queueingServiceAddress.sin_family = AF_INET;
	queueingServiceAddress.sin_addr.s_addr = inet_addr(IP_ADDRESS);
	queueingServiceAddress.sin_port = htons(portNumber);
	int sockAddrLen = sizeof(struct sockaddr);

	connectSocket = connectToSocket((struct sockaddr*)&queueingServiceAddress, sockAddrLen);
	if (connectSocket == INVALID_SOCKET)
	{
		printf("Greska prilikom povezivanja.\n");
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	if (portNumber == QUEUEINGSERVICE_PORT1)//opet gledamo koji je
	{
		printf("Uspesno konektovan na Queueing Servis 1.\n");
	}
	else if (portNumber == QUEUEINGSERVICE_PORT2)
	{
		printf("Uspesno konektovan na Queueing Servis 2.\n");
	}

	int recieved = 0;
	MESSAGE* message = (MESSAGE*)malloc(sizeof(MESSAGE));
	printf("Pritisnite taster za unos poruka\n");
	while (1)
	{
		if (_kbhit()) {//kbhit funkija ako unesemo neki taster registruje kao tru

			memset(message->processName, 0, PROCESS_STRING_SIZE);//inicijalizujemo mess na prazne podatke
			memset(message->data, 0, DATA_SIZE);
			message->type = PROCESS_DATA;//kazemo da je procesdata u pitanju

			memcpy(message->processName, argv[1], strlen(argv[1]));
			printf("Unesite poruku: \n");
			gets(message->data);//radimo unos sa tasture

			sendData(connectSocket, (char*)message, sizeof(MESSAGE));//saljemo kjuing servisu poruku
		}

		recieved = receiveData(connectSocket, (char*)message, sizeof(MESSAGE), 1);//pozivamo receive
		if (recieved == sizeof(MESSAGE))//ako smo primili pporuku
		{
			printf("Primljeno - Za proces: %s Poruka: %s\n", message->processName, message->data);
		}
		else if (recieved != -2)
		{
			printf("Connection closed.\n");
			closesocket(connectSocket);
			WSACleanup();
			return 1;
		}

		Sleep(50);
	}

	
	printf("Pritisnite taster za izlaz iz programa: \n");
	getchar();

	return 0;
}