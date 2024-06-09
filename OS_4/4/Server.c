#define _DEFAULT_SOURCE
#include <stdio.h>  /* for printf() and fprintf() */
#include <stdlib.h> /* for atoi() and exit() */
#include <string.h> /* for memset() */
#include <unistd.h> /* for close() */
#include <time.h>   /* for time() */
#include <signal.h>

#include "List.h"
#include "Book.h"
#include "Task.h"
#include "IO.h"

#define MSGMAX 255 /* Longest message string */
#define ADDRLEN 24

typedef struct List Catalog;   // Ordered list of the books
typedef struct List TaskQueue; // Queue of the tasks

typedef struct Observer
{
    struct sockaddr_in addr;
} Observer;

typedef struct PendingTask
{
    Task task;
    time_t time;
} PendingTask;

typedef struct List PendingTaskQueue; // Queue of the tasks

// Structure to store all system variables
typedef struct Library
{
    Catalog *catalog;    // Ordered list of books
    int catalogFullSize; // M * N * K
    TaskQueue *taskQueue;
    PendingTaskQueue *pendingTaskQueue;
    int sock;
    List *observers;
} Library;

Library library; /* GLOBAL for signal handler */

/* Initializes the library*/
void Initialize(Library *library, int M, int N, int K);

void UpdateQueues(Library *library);

int ObserverCompare(const void *, const void *);

void NotifyObservers(Library *library, const char *msg);

int TaskCompare(const void *, const void *);

/* Parse message from client*/
int ParseMessage(char *msg, Book *book);

/* Print Catalog */
void PrintCatalog(Library *library);

void DieWithError(char *errorMessage); /* Error handling function */
void UseIdleTime();                    /* Function to use idle time */
void SIGIOHandler(int signalType);     /* Function to handle SIGIO */

int main(int argc, char *argv[])
{
    /* Test for correct number of parameters */
    if (argc != 5)
    {
        fprintf(stderr, "Usage:  %s <SERVER PORT> <M> <N> <K>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    unsigned short libServPort = atoi(argv[1]); /* First arg:  local port */

    const int M = atoi(argv[2]);
    const int N = atoi(argv[3]);
    const int K = atoi(argv[4]);

    Initialize(&library, M, N, K);

    library.sock = CreateUDPServerWithSIGIO(libServPort, SIGIOHandler);

    /* Go off and do real work; message receiving happens in the background */

    for (;;)
        UseIdleTime();

    /* NOTREACHED */
}

void Initialize(Library *library, int M, int N, int K)
{
    library->catalog = ListCreate();
    library->taskQueue = ListCreate();
    library->pendingTaskQueue = ListCreate();
    library->observers = ListCreate();
    library->catalogFullSize = M * N * K;

    // Fill task queue
    for (int m = 0; m < M; ++m)
    {
        for (int n = 0; n < N; ++n)
        {
            for (int k = 0; k < K; ++k)
            {
                Task *task = TaskCreate(m, n, k);
                ListPushBack(library->taskQueue, task);
            }
        }
    }
}

int ObserverCompare(const void *a, const void *b)
{
    Observer *obs1 = (Observer *)a;
    Observer *obs2 = (Observer *)b;
    if (obs1->addr.sin_addr.s_addr == obs2->addr.sin_addr.s_addr && obs1->addr.sin_port == obs2->addr.sin_port)
        return 0;
    return -1;
}

int TaskCompare(const void *a, const void *b)
{
    PendingTask *t1 = (PendingTask *)a;
    PendingTask *t2 = (PendingTask *)b;
    if (t1->task.m == t2->task.m && t1->task.n == t2->task.n && t1->task.k == t2->task.k)
        return 0;
    return -1;
}

void NotifyObservers(Library *library, const char *msg)
{
    Node *iter = library->observers->head;
    while (iter)
    {
        Observer *obs = (Observer *)iter->payload;
        SendTo(library->sock, msg, strlen(msg), &obs->addr);
        iter = iter->next;
    }
    printf("%s\n", msg);
}

int ParseMessage(char *msg, Book *book)
{
    if (sscanf(msg, "%d:%d:%d:%d", &book->id, &book->pos.m, &book->pos.n, &book->pos.k) != 4)
    {
        // Invalid message
        return 0;
    }
    return 1;
}

void PrintCatalog(Library *library)
{
    char notifyBuffer[MSGMAX];

    NotifyObservers(library, "The recovered catalog is:");
    Node *node = library->catalog->head;
    while (node)
    {
        Book *book = (Book *)node->payload;
        sprintf(notifyBuffer, "%d - %d, %d, %d", book->id, book->pos.m, book->pos.n, book->pos.k);
        NotifyObservers(library, notifyBuffer);
        node = node->next;
    }
}

void UseIdleTime()
{
    // Moves uncompleted tasks from pending queue to task queue
    UpdateQueues(&library);

    printf(".\n");
    sleep(5); /* 5 seconds of activity */
}

void SIGIOHandler(int signalType)
{
    struct sockaddr_in clientAddr; /* Address of datagram source */
    int recvMsgSize;               /* Size of datagram */
    char msgBuffer[MSGMAX];        /* Datagram buffer */
    char addrBuffer[ADDRLEN];
    char notifyBuffer[MSGMAX];

    int sock = library.sock;

    do
    {
        /* As long as there is input... */

        // Receive message from client
        recvMsgSize = MSGMAX;
        if (RecvFromUnblocked(sock, msgBuffer, &recvMsgSize, &clientAddr))
        {
            /* null-terminate the received data */
            msgBuffer[recvMsgSize] = '\0';

            printf("Handling client %s:%d...\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
            sprintf(addrBuffer, "%s:%d", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

            if (strcmp(msgBuffer, "I_AM_OBSERVER") == 0)
            {
                // This is the first message from the observer client

                Observer obs;
                obs.addr = clientAddr;

                if (!ListContains(library.observers, &obs, ObserverCompare))
                {
                    // Add a new observer
                    Observer *obsItem = malloc(sizeof(obsItem));
                    obsItem->addr = clientAddr;
                    ListPushBack(library.observers, obsItem);

                    sprintf(notifyBuffer, "Client %s is registered as observer", addrBuffer);
                    NotifyObservers(&library, notifyBuffer);
                }

                continue;
            }
            else if (strcmp(msgBuffer, "DISCONNECT") == 0)
            {
                sprintf(notifyBuffer, "Client %s will be disconnected", addrBuffer);
                NotifyObservers(&library, notifyBuffer);

                // Check if an observer wants to disconnect
                Observer obs;
                obs.addr = clientAddr;
                if (ListContains(library.observers, &obs, ObserverCompare))
                {
                    // Remove observer from list
                    Observer *obsItem = ListRemove(library.observers, &obs, ObserverCompare);
                    free(obsItem);
                }
                else
                {
                    // an worker wants to disconnect
                }

                continue;
            }
            else if (strcmp(msgBuffer, "GIVE_ME_TASK") == 0)
            {
                // This is the first message from the worker client
                sprintf(notifyBuffer, "Client %s requests a task", addrBuffer);
                NotifyObservers(&library, notifyBuffer);
            }
            else
            {
                // This is not the first message.
                // The client must send the ID of the book found at the position given to it.

                Book b;
                if (ParseMessage(msgBuffer, &b) == 0)
                {
                    // skip invalid message
                    printf("Warning! Invalid message received: \"%s\"\n", msgBuffer);
                    continue;
                }

                // Remove pending task from the list
                PendingTask pt;
                pt.task = b.pos;
                void *item = ListRemove(library.pendingTaskQueue, &pt, TaskCompare);
                free(item);

                sprintf(notifyBuffer, "Client %s found book %d at position (%d, %d, %d)", addrBuffer, b.id, b.pos.m, b.pos.n, b.pos.k);
                NotifyObservers(&library, notifyBuffer);

                // Create a new book item and insert it into catalog
                Book *bookItem = BookCreate(b.id, b.pos.m, b.pos.n, b.pos.k);
                if (!ListContains(library.catalog, bookItem, BookCompare))
                {
                    ListInsert(library.catalog, bookItem, BookCompare);
                }

                // Check if catalog is completely recovered
                if (ListSize(library.catalog) == library.catalogFullSize)
                {
                    PrintCatalog(&library);
                }
            }

            if (ListEmpty(library.taskQueue))
            {
                // No more tasks
                // printf("  No more tasks, all books have been found.\n");
                // sprintf(msgBuffer, "NO_MORE_TASKS");
                continue;
            }
            else
            {
                // Extract the next task from the queue
                Task *task = ListPopFront(library.taskQueue);
                sprintf(notifyBuffer, "Sending next task (%d, %d, %d) to client %s", task->m, task->n, task->k, addrBuffer);
                NotifyObservers(&library, notifyBuffer);
                TaskCreateMessage(msgBuffer, task);

                // Create pending task
                PendingTask *ptItem = malloc(sizeof(*ptItem));
                ptItem->task = *task;
                ptItem->time = time(NULL);
                free(task);
                ListPushBack(library.pendingTaskQueue, ptItem);
            }

            // Send next task
            SendTo(sock, msgBuffer, strlen(msgBuffer), &clientAddr);
        }
    } while (recvMsgSize >= 0);
    /* Nothing left to receive */
}

// Moves uncompleted tasks from pending queue to task queue
void UpdateQueues(Library *library)
{
    // Temporary block all signals
    sigset_t sigblock;
    sigfillset(&sigblock);
    sigprocmask(SIG_BLOCK, &sigblock, NULL);

    time_t now = time(NULL);

    // Move uncompleted tasks from pending queue to task queue
    int flag;
    while (1)
    {
        flag = 0;

        Node *iter = library->pendingTaskQueue->head;
        while (iter)
        {
            PendingTask *pt = (PendingTask *)iter->payload;
            if (now - pt->time > 5)
            {
                // Reinsert old uncompleted task into task queue
                Task *task = TaskCreate(pt->task.m, pt->task.n, pt->task.k);
                ListPushBack(library->taskQueue, task);

                // Remove uncompleted task from pending task queue
                pt = ListRemove(library->pendingTaskQueue, pt, TaskCompare);
                free(pt);
                flag = 1;
                break;
            }
            iter = iter->next;
        }

        if (flag == 0)
            break;
    }
    // Unblock signals
    sigprocmask(SIG_UNBLOCK, &sigblock, NULL);
}
