#define _DEFAULT_SOURCE
#include <stdio.h>  /* for printf() and fprintf() */
#include <stdlib.h> /* for atoi() and exit() */
#include <string.h> /* for memset() */
#include <unistd.h> /* for close() */

#include "List.h"
#include "Book.h"
#include "Task.h"
#include "IO.h"

#define MSGMAX 255 /* Longest message string */

typedef struct List Catalog;   // Ordered list of the books
typedef struct List TaskQueue; // Queue of the tasks

// Structure to store all system variables
typedef struct Library
{
    Catalog *catalog; // Ordered list of books
    int catalogFullSize; // M * N * K
    TaskQueue *taskQueue;
    int sock;
} Library;

Library library; /* GLOBAL for signal handler */

/* Initializes the library*/
void Initialize(Library *library, int M, int N, int K);

/* Parse message from client*/
int ParseMessage(char *msg, Book *book);

/* Print Catalog */
void PrintCatalog(Catalog *catalog);

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

int ParseMessage(char *msg, Book *book)
{
    if (sscanf(msg, "%d:%d:%d:%d", &book->id, &book->pos.m, &book->pos.n, &book->pos.k) != 4)
    {
        // Invalid message
        return 0;
    }
    return 1;
}

void PrintCatalog(Catalog *catalog)
{
    Node *node = catalog->head;
    while (node)
    {
        Book *book = (Book *)node->payload;
        printf("%d - %d, %d, %d\n", book->id, book->pos.m, book->pos.n, book->pos.k);
        node = node->next;
    }
}

void UseIdleTime()
{
    printf(".\n");
    sleep(10); /* 10 seconds of activity */
}

void SIGIOHandler(int signalType)
{
    struct sockaddr_in clientAddr; /* Address of datagram source */
    int recvMsgSize;               /* Size of datagram */
    char msgBuffer[MSGMAX];        /* Datagram buffer */

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

            if (strcmp(msgBuffer, "GIVE_ME_TASK") == 0)
            {
                // This is the first message from the client
                printf("  Client requests a task\n");
            }
            else
            {
                // This is not the first message.
                // The client must send the ID of the book found at the position given to it.

                Book b;
                if (ParseMessage(msgBuffer, &b) == 0)
                {
                    // skip invalid message
                    printf("  Invalid message received: \"%s\"\n", msgBuffer);
                    continue;
                }

                printf("  Client found book %d at position (%d, %d, %d)\n", b.id, b.pos.m, b.pos.n, b.pos.k);

                // Create a new book item and insert it into catalog
                Book *bookItem = BookCreate(b.id, b.pos.m, b.pos.n, b.pos.k);
                if (!ListContains(library.catalog, bookItem, BookCompare))
                {
                    ListInsert(library.catalog, bookItem, BookCompare);
                }                

                // Check if catalog is completely recovered
                if (ListSize(library.catalog) == library.catalogFullSize)
                {
                    printf("The recovered catalog is:\n");
                    PrintCatalog(library.catalog);
                }
            }

            if (ListEmpty(library.taskQueue))
            {
                // No more tasks
                printf("  No more tasks, all books have been found.\n");                
                // sprintf(msgBuffer, "NO_MORE_TASKS");
                continue;
            }
            else
            {
                Task *task = ListPopFront(library.taskQueue);
                printf("  Sending next task (%d, %d, %d)\n", task->m, task->n, task->k);
                TaskCreateMessage(msgBuffer, task);
                free(task);
            }

            // Send next task
            SendTo(sock, msgBuffer, strlen(msgBuffer), &clientAddr);
        }
    } while (recvMsgSize >= 0);
    /* Nothing left to receive */
}
