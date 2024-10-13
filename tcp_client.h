#ifndef TCP_CLIENT_H_   /* Include guard */
#define TCP_CLIENT_H_

#include "packet.h"

void client_loop(char * addr, int port, struct packet **payload, size_t len);

int dial(char * addr, int port, struct packet * connect,  int (*callback)(struct packet * pkt, unsigned char *));
#endif // TCP_CLIENT_H_
