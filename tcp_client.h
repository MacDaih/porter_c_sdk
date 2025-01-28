#ifndef TCP_CLIENT_H_   /* Include guard */
#define TCP_CLIENT_H_

#include "packet.h"

int dial_start(int sockfd, char * addr, int port, context ctx, struct packet * connect);
#endif // TCP_CLIENT_H_
