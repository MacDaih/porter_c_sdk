#ifndef TCP_CLIENT_H_   /* Include guard */
#define TCP_CLIENT_H_

#include "packet.h"

int dial_start(char * addr, int port, context ctx, struct packet * connect,  int (*callback)(unsigned char *));
#endif // TCP_CLIENT_H_
