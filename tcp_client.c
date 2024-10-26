#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
                    //
#include "packet.h"

#define MAX 1024

int dial_start(
        char * addr,
        int port,
        context ctx,
        struct packet * p,
        packet * (*callback)(unsigned char *),
) {
    packet * cursor = p;
    
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;
 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        return 1;
    }

    bzero((char *) &servaddr, sizeof(servaddr));
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(addr);
    servaddr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("connection with the server failed...\n");
        return 1;
    }

    unsigned char buff[MAX];
    while(cursor) {
        if(int r = write(sockfd, cursor->payload, 1024);  r < 0) return 1;

        if(int r = read(sockfd, buff, sizeof(buff)); r < 0) return 1;
        
        // read packet callback
        packet * np = callback(ctx, buff);

        // append to packet list
        if(np != NULL) {
            np->next = cursor->next;
            cursor->next = np;
        }
        cursor = cursor->next;
    }
    return 0;
}
