#include "QueueingService.h"

DWORD __stdcall receiveQueingServiceAndProcessData(LPVOID param)
{
    THREAD1_PARAM* parameter = (THREAD1_PARAM*)param;//kastujemo ga na pokazivac na strukturu
    SOCKET* processSockets = parameter->processesSockets;//izvucemo podatke koji su nam neophodni
    int processNum = parameter->processNum;
    SOCKET queueingServiceSocket = parameter->queueingServiceSocket;
    QUEUE* queue = parameter->queue;
    QUEUEARRAY* queueArray = parameter->queueArray;

    int iResult = 0;
    unsigned long mode = 1;
    MESSAGE* message = (MESSAGE*)malloc(sizeof(MESSAGE));//inicijalizujemo memoriju za poruku gde cemo svaki put da smestamo poruku kad je primimo pa cemo posle da vidimo sta cemo sa njom

    while (1)
    {
        fd_set readfds;//setovi za citanje podataka
        FD_ZERO(&readfds);//inicijalizujemo ga 
        FD_SET(queueingServiceSocket, &readfds);//ubaicmo sokete u set

        for (int i = 0; i < processNum; ++i)
            FD_SET(processSockets[i], &readfds);

        struct timeval timeVal;
        timeVal.tv_sec = 1;
        timeVal.tv_usec = 0;

        int result = select(0, &readfds, NULL, NULL, &timeVal);

        if (result == SOCKET_ERROR)
        {
            printf("There was an error while connecting to the socket");
            return 1;
        }
        else if (result > 0)//ako selekt vrati >0 postoji barem jedan soket koji je spreman da primi podatke
        {
            if (FD_ISSET(queueingServiceSocket, &readfds))
            {
                int received = receiveData(queueingServiceSocket, (char*)message, sizeof(MESSAGE), 0);//pozovemo recdat, prosledimo soket,poruku

                if (received >= (int)sizeof(int))
                {
                    if (message->type == PROCESS_DATA)
                    {
                        push(queue, message);
                    }
                    else if (message->type == QUEUEING_SERVICE_DATA)
                    {
                        insertInQueueArray(queueArray, message);//ubacimo u kjuarr
                    }
                }
            }

            for (int i = 0; i < processNum; ++i)
            {
                if (FD_ISSET(processSockets[i], &readfds))//isto proverimo za svaki od process skoeta, da li se nalaze u setu
                {
                    int received = receiveData(processSockets[i], (char*)message, sizeof(MESSAGE), 0);

                    if (received >= (int)sizeof(int)), //moze i size of message
                    {
                        if (message->type == PROCESS_DATA)//ako je poruka stigla od nekogprocesa smestimo je na kju
                        {
                            push(queue, message);
                        }
                        else if (message->type == QUEUEING_SERVICE_DATA)
                        {
                            insertInQueueArray(queueArray, message);
                        }
                    }
                }
            }
        }
    }               
}

DWORD __stdcall sendDataToQueueingService(LPVOID param)
{
    THREAD2_PARAM* parameter = (THREAD2_PARAM*)param;
    SOCKET queueingServiceSocket = parameter->queueingServiceSocket;
    QUEUE* queue = parameter->queue;

    MESSAGE* message = (MESSAGE*)malloc(sizeof(MESSAGE));

    while (1)
    {
        if (pop(queue, message))//ako pop vrati tru nesto je bilo na kju i prosledimo to
        {
            message->type = QUEUEING_SERVICE_DATA;
            sendData(queueingServiceSocket, (char*)message, BUFFER_SIZE);//proseldjujemo kjuing servisu bilo sta sto je na kju
        }
        else
        {
            Sleep(50);//u suprotonm imamo sleep
        }
    }

    free(message);
    return 0;
}

DWORD __stdcall sendDataToProcesses(LPVOID param)
{
    THREAD3_PARAM* parameter = (THREAD3_PARAM*)param;
    QUEUEARRAY* queueArray = parameter->queueArray;

    MESSAGE* message = (MESSAGE*)malloc(sizeof(MESSAGE));
    int queueArraySize;
    while (1)
    {
        queueArraySize = getQueueArraySize(queueArray);//uzimamo sajz

        for (int i = 0; i < queueArraySize; i++)
        {
            while (pop(&(queueArray->processQueues[i]), message))//na pop proseldimo kju i poruku sto ce da se smesti, ako je vratio tru tacno znamo kome treba da prosledimo
            {
                message->type = PROCESS_DATA;
                sendData(queueArray->processSockets[i], (char*)message, sizeof(MESSAGE));
            }
        }

        Sleep(100);
    }

    free(message);
    return 0;
}
