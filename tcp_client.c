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

int dial(char * addr, int port, struct packet * p, int (*callback)(struct packet * pkt, unsigned char *)) {
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;
 
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        return 1;
    }
//    else
//        printf("Socket successfully created..\n");
 

    bzero((char *) &servaddr, sizeof(servaddr));
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(addr);
    servaddr.sin_port = htons(port);
    // connect the client socket to server socket
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("connection with the server failed...\n");
        return 1;
    }
//    else
//        printf("connected to the server..\n");
 
    unsigned char buff[MAX];
    
    write(sockfd, p->payload, 1024);

    read(sockfd, buff, sizeof(buff));

    while(callback(p->next, (unsigned char *)(&buff)) == 1) {
        write(sockfd, buff, sizeof(buff));
        if(buff[0] == 0xe0 || buff[0] == 0x30) break;

        bzero(&buff, sizeof(buff));
        read(sockfd, buff, sizeof(buff));
    }

    close(sockfd);
    return 0;
}
