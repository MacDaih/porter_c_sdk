#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
                
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

    // TODO Set timeout 
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

    struct timeval tv = {5, 0}; // TODO use timeout value from context
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval));
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("connection with the server failed...\n");
        return 1;
    }

    unsigned char buff[MAX];
    while(cursor) {
        if(send(sockfd, cursor->payload, sizeof(cursor->payload)) < 1) return 1;
        
        int ret = recv(sockfd, buff, sizeof(buff)); r < 0)
        if (ret == -1 && errno == EAGAIN) {
            // ping server
            packet * ping = (packet *) malloc(sizeof(packet));
            make_ping(ping);
            if(send(sockfd, ping->payload, sizeof(ping->payload)) < 1) return 1;
            free(ping);
        }

        // read packet callback
        packet * np = callback(ctx, buff);

        // append to packet list
        if(np != NULL) {
            np->next = cursor->next;
            cursor->next = np;
        }
        cursor = cursor->next;
        bzero(buff, sizeof(buff));
    }
    return 0;
}
