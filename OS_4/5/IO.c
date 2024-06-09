#define _DEFAULT_SOURCE

#include "IO.h"
#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind, and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() and getpid() */
#include <fcntl.h>      /* for fcntl() */
#include <sys/file.h>   /* for O_NONBLOCK and FASYNC */
#include <signal.h>     /* for signal() and SIGALRM */
#include <errno.h>      /* for errno */

void DieWithError(char *errorMessage); /* External error handling function */

int CreateUDPServerWithSIGIO(unsigned short serverPort, void (*SIGIOHandler)(int))
{
    struct sockaddr_in serverAddr; /* Server address */
    int sock;
    struct sigaction handler;      /* Signal handling action definition */

    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Set up the server address structure */
    memset(&serverAddr, 0, sizeof(serverAddr));     /* Zero out structure */
    serverAddr.sin_family = AF_INET;                /* Internet family */
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    serverAddr.sin_port = htons(serverPort);       /* Port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        DieWithError("bind() failed");

    /* Set signal handler for SIGIO */
    handler.sa_handler = SIGIOHandler;
    /* Create mask that mask all signals */
    if (sigfillset(&handler.sa_mask) < 0)
        DieWithError("sigfillset() failed");
    /* No flags */
    handler.sa_flags = 0;

    if (sigaction(SIGIO, &handler, 0) < 0)
        DieWithError("sigaction() failed for SIGIO");

    /* We must own the socket to receive the SIGIO message */
    if (fcntl(sock, F_SETOWN, getpid()) < 0)
        DieWithError("Unable to set process owner to us");

    /* Arrange for nonblocking I/O and SIGIO delivery */
    if (fcntl(sock, F_SETFL, O_NONBLOCK | FASYNC) < 0)
        DieWithError("Unable to put client sock into non-blocking/async mode");
    
    return sock;
}

void SendTo(int sock, const char *msg, int msgLen, const struct sockaddr_in *addr)
{
    if (sendto(sock, msg, msgLen, 0, (struct sockaddr *)addr, sizeof(*addr)) != msgLen)
        DieWithError("sendto() sent a different number of bytes than expected");
}

void RecvFrom(int sock, char *msg, int *msgLen, struct sockaddr_in *addr)
{
    unsigned int addrLen = sizeof(*addr);
    *msgLen = recvfrom(sock, msg, *msgLen, 0, (struct sockaddr *)addr, &addrLen);
    if (*msgLen < 0)
        DieWithError("recvfrom() failed");
}

int RecvFromUnblocked(int sock, char *msg, int *msgLen, struct sockaddr_in *addr)
{
    unsigned int addrLen = sizeof(*addr);
    *msgLen = recvfrom(sock, msg, *msgLen, 0, (struct sockaddr *)addr, &addrLen);
    if (*msgLen < 0)
    {
        /* Only acceptable error: recvfrom() would have blocked */
        if (errno != EWOULDBLOCK)
            DieWithError("recvfrom() failed");
        return 0;
    }
    return 1;
}
