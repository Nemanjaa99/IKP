#include "Queue.h"

void initializeQueueArray(QUEUEARRAY* queueArray)
{
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        queueArray->processSockets[i] = INVALID_SOCKET;//svaki soket na invalid
        initializeQueue(&(queueArray->processQueues[i]));//inicijalizujemo kju
    }

    queueArray->size = 0;
    InitializeCriticalSection(&(queueArray->cs));//inicijalizujemo kriticnu sekciju

}

int getQueueArraySize(QUEUEARRAY* queueArray)
{
    return queueArray->size;
}

void insertInQueueArray(QUEUEARRAY* queueArray, MESSAGE* message)
{
    EnterCriticalSection(&(queueArray->cs));

    // Pronadji socket
    int i = 0;
    for (i = 0; i < ARRAY_SIZE; i++)
    {
        if (strcmp(queueArray->processNames[i], message->processName) == 0) //imamo potrebu da ubacimo poruku u kju array
        {//pokusavamo da nadjemo ime reda, kada ga nadjemo i je indeks gde se nalazi kju
            break;
        }
    }

    LeaveCriticalSection(&(queueArray->cs));

    // Dodavanje elementa na Queue
    push(&(queueArray->processQueues[i]), message);


}

int addProcessToQueueArray(QUEUEARRAY* queueArray, char* processName, SOCKET s)
{
    EnterCriticalSection(&(queueArray->cs));

    int i = 0;
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        if (queueArray->processSockets[i] == INVALID_SOCKET)//innicijalizovali smo ove sokete na invalid soket, kada vidimo da je invalid soket
        {
            queueArray->processSockets[i] = s; // dodamo soket
            memcpy((char*)(queueArray->processNames) + i * PROCESS_STRING_SIZE, processName, PROCESS_STRING_SIZE); //kopiramo proces name u queueArray->processNames
            break;
        }
    }

    queueArray->size++;//povecamo sajz i prakticno smo dodali proces u kjuarray

    LeaveCriticalSection(&(queueArray->cs));
}

void initializeQueue(QUEUE* queue)
{
    queue->front = NULL;
    queue->tail = NULL;
    InitializeCriticalSection(&(queue->cs));
}

int pop(QUEUE* queue, MESSAGE* message)
{
    EnterCriticalSection(&(queue->cs));

    if (queue->tail == NULL)
    {
        LeaveCriticalSection(&(queue->cs));
        return 0;
    }

    if (queue->front == queue->tail)//
    {
        NODE* popNode = queue->tail;

        // Novi tail i front su NULL jer je jedini element
        queue->front = NULL;
        queue->tail = NULL;

        // Kopira se sadrzaj poruke u message
        memcpy(message, popNode, sizeof(MESSAGE));//kopiramo ga u ovu memoriju koja je prosledjena funkciji, gde zelimo da sacuvamo element
        free(popNode);

        LeaveCriticalSection(&(queue->cs));
        return 1;
    }

    NODE* popNode = queue->tail;
    
    // Novi tail je prethodni
    NODE* newTail = queue->tail->prev;
    queue->tail = newTail;
    newTail->next = NULL;

    // Kopira se sadrzaj poruke u message
    memcpy(message, popNode, sizeof(MESSAGE));

    free(popNode);

    LeaveCriticalSection(&(queue->cs));
    return 1;
}

void push(QUEUE* queue, MESSAGE* message)
{
    EnterCriticalSection(&(queue->cs)); //vise nitna aplikacija ako pristupaju istim podacima, cim menjamo sadrzaj kjua, imamo potrebu da zakljucamo sve

    NODE* newNode = (NODE*)malloc(sizeof(NODE));// innicijalizujemo memoriju za novi node
    // Inicijalizacija pokazivaca
    newNode->next = NULL;
    newNode->prev = NULL;
    // Kopiraju se podaci u poruku cvora
    memcpy(&(newNode->message), message, sizeof(MESSAGE)); // kopiramo poruku u poruku gde su podaci koji se salju koja se salje

    if (queue->front == NULL)
    {
        queue->front = newNode;
        queue->tail = newNode;
        LeaveCriticalSection(&(queue->cs));
        return;
    }

    NODE* temp = queue->front;
    // Unos na pocetak liste
    queue->front = newNode;
    newNode->next = temp;
    temp->prev = newNode;

    LeaveCriticalSection(&(queue->cs));
    return;
}
