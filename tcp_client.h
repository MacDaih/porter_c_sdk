#ifndef TCP_CLIENT_H_   /* Include guard */
#define TCP_CLIENT_H_

#include "packet.h"

int dial_start(char * addr, int port, context ctx, struct packet * connect,  struct packet * (*callback)(context ctx,unsigned char * payload));
#endif // TCP_CLIENT_H_
