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

void DieWithError(char *errorMessage); /* External error handling function */

void SIGINTHandler(int);

int sock;                       /* Socket descriptor - GLOBAL for SIGINTHandler */
struct sockaddr_in libServAddr; /* Library server address - GLOBAL for SIGINTHandler */

int main(int argc, char *argv[])
{
    struct sockaddr_in fromAddr; /* Source address of response */
    unsigned short libServPort;  /* Library server port */
    char *servIP;                /* IP address of server */
    char buffer[MSGMAX + 1];     /* Buffer for storing message */
    int msgLen;                  /* Length of received response */
    struct sigaction handler;    /* Signal handling action definition */

    if (argc != 3) /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage: %s <Server IP> <Server Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    servIP = argv[1];            /* First arg: server IP address (dotted quad) */
    libServPort = atoi(argv[2]); /* Second arg: server port */

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

    // Send initial message "I_AM_OBSERVER"
    sprintf(buffer, "I_AM_OBSERVER");
    SendTo(sock, buffer, strlen(buffer), &libServAddr);

    for (;;)
    {
        msgLen = MSGMAX;
        RecvFrom(sock, buffer, &msgLen, &fromAddr);

        if (libServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr)
        {
            fprintf(stderr, "Warning: received a packet from unknown source.\n");
            continue;
        }

        buffer[msgLen] = '\0';

        if (strcmp(buffer, "NO_MORE_TASKS") == 0)
        {
            break;
        }

        printf("%s\n", buffer);
    }

    printf("The observer is shutting down.\n");

    close(sock);
    exit(EXIT_SUCCESS);
}

void SIGINTHandler(int signalType)
{
    printf("\nSIGINT received, notify the server.\n");
    SendTo(sock, "DISCONNECT", strlen("DISCONNECT"), &libServAddr);
    exit(EXIT_SUCCESS);
}
