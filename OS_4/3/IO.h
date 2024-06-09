#ifndef IO_H
#define IO_H

#include <arpa/inet.h>  /* for sockaddr_in */

int CreateUDPServerWithSIGIO(unsigned short serverPort, void (*SIGIOHandler)(int));

void SendTo(int sock, const char *msg, int msgLen, const struct sockaddr_in *addr);

void RecvFrom(int sock, char *msg, int *msgLen, struct sockaddr_in *addr);

int RecvFromUnblocked(int sock, char *msg, int *msgLen, struct sockaddr_in *addr);

#endif
