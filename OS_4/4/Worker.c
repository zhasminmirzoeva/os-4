#define _DEFAULT_SOURCE

#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi(), rand() and exit() */
#include <time.h>       /* for time()*/
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() and usleep() */
#include <signal.h>     /* for signal() and SIGALRM */

#include "Book.h"
#include "Task.h"
#include "IO.h"

#define MSGMAX 255 /* Longest message string */

void SIGINTHandler(int);

void DieWithError(char *errorMessage); /* External error handling function */
// Parse the book from one line of the input file
void ParseBook(char *line, Book *book);
// Find the book in the input file by the given position (task)
int FindBook(const char *filename, const Task *task, Book *book);

int sock;                       /* Socket descriptor - GLOBAL for SIGINTHandler */
struct sockaddr_in libServAddr; /* Library server address - GLOBAL for SIGINTHandler */

int main(int argc, char *argv[])
{    
    struct sockaddr_in fromAddr;    /* Source address of response */
    unsigned short libServPort;     /* Library server port */
    char *servIP;                   /* IP address of server */
    char *libFilename;              /* Filename containing positions of the books in the library */
    char outBuffer[MSGMAX + 1];     /* Buffer for storing request */
    char inBuffer[MSGMAX + 1];      /* Buffer for receiving response */
    int responseLen;                /* Length of received response */
    struct sigaction handler;    /* Signal handling action definition */

    if (argc != 4) /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage: %s <Server IP> <Server Port> <Library Filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    servIP = argv[1];            /* First arg: server IP address (dotted quad) */
    libServPort = atoi(argv[2]); /* Second arg: server port */
    libFilename = argv[3];       /* Third arg */

    /* Set signal handler for SIGTERM */
    handler.sa_handler = SIGINTHandler;
    /* Create mask that mask all signals */
    if (sigfillset(&handler.sa_mask) < 0)
        DieWithError("sigfillset() failed");
    /* No flags */
    handler.sa_flags = 0;

    if (sigaction(SIGINT, &handler, 0) < 0)
        DieWithError("sigaction() failed for SIGINT");

    /* Create a datagram/UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&libServAddr, 0, sizeof(libServAddr));    /* Zero out structure */
    libServAddr.sin_family = AF_INET;                /* Internet addr family */
    libServAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
    libServAddr.sin_port = htons(libServPort);       /* Server port */

    // Send initial message "GIVE_ME_TASK"
    sprintf(outBuffer, "GIVE_ME_TASK");
    SendTo(sock, outBuffer, strlen(outBuffer), &libServAddr);

    for (;;)
    {
        responseLen = MSGMAX;
        RecvFrom(sock, inBuffer, &responseLen, &fromAddr);

        if (libServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr)
        {
            fprintf(stderr, "Warning: received a packet from unknown source.\n");
            continue;
        }

        inBuffer[responseLen] = '\0';

        // Parse the task from the message
        Task task;
        TaskParse(inBuffer, &task);

        printf("Received task: (%d, %d, %d)\n", task.m, task.n, task.k);

        Book book;

        // Find the book ID in the input file
        if (FindBook(libFilename, &task, &book))
        {
            printf("  Book %d found at (%d, %d, %d)\n", book.id, task.m, task.n, task.k);
            sprintf(outBuffer, "%d:%d:%d:%d", book.id, book.pos.m, book.pos.n, book.pos.k);
            // Send found book to the server
            SendTo(sock, outBuffer, strlen(outBuffer), &libServAddr);
        }
        else
        {
            // not found, most likely an error
            printf("  Nothing found\n");
        }
    }

    close(sock);
    exit(EXIT_SUCCESS);
}

void ParseBook(char *line, Book *book)
{
    if (sscanf(line, "%d:%d:%d:%d", &book->pos.m, &book->pos.n, &book->pos.k, &book->id) != 4)
    {
        fprintf(stderr, "Invalid file format\n");
        exit(EXIT_FAILURE);
    }
}

int FindBook(const char *filename, const Task *task, Book *book)
{
    int result = 0;

    // Open file for reading
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Unable to open file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    char line[MSGMAX + 1];

    // Check each line in the input file
    while (fgets(line, MSGMAX + 1, fp))
    {
        ParseBook(line, book);
        if (book->pos.m == task->m && book->pos.n == task->n && book->pos.k == task->k)
        {
            // Book found
            result = 1;
            break;
        }
    }

    fclose(fp);

    // Generate a random delay from 1000 to 3000 ms
    srand(time(NULL));
    int ms = 1000 + rand() % 2001;
    usleep(ms * 1000);

    return result;
}

void SIGINTHandler(int signalType)
{
    printf("\nSIGINT received, notify the server.\n");
    SendTo(sock, "DISCONNECT", strlen("DISCONNECT"), &libServAddr);
    exit(EXIT_SUCCESS);
}
