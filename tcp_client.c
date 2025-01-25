#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <errno.h>

#include "packet.h"

#define MAX 1024

int dial_start(
        char * addr,
        int port,
        context ctx,
        struct packet * p
) {
    struct packet * cursor = p;
    
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        return 1;
    }

    bzero((char *) &servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(addr);
    servaddr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("failed to connect to server %s\n", strerror(errno));    
        close(sockfd);
        return 1;
    }

    unsigned char buff[MAX];

    struct pollfd fd;
    fd.fd = sockfd;
    fd.events = POLLIN;

    while(cursor) {
        printf("cursor packet %x\n",cursor->payload[0]);
        int m_res = write(sockfd, cursor->payload, cursor->len);
        if(m_res < 0) {
            printf("failed to write to server %s\n", strerror(errno));    
            close(sockfd);
            return 1; 
        }
         
        struct packet * np = NULL;
        if(poll(&fd, 1, (int)(ctx.keep_alive * 1000)) > 0) {
          int r_res = read(sockfd,buff,sizeof(buff));
          if(r_res < 0) {
              close(sockfd);
              return 1;
          }
        } else if (ctx.keep_alive > 0) {
            struct packet * ping = new_packet();
            make_ping(ping);

            bzero(buff, sizeof(buff));
            if(write(sockfd, ping->payload, ping->len)) {
                printf("failed to write ping to server %s\n", strerror(errno));    
                close(sockfd);
                return 1; 
            }

            free(ping);
            int r_res = read(sockfd, buff, sizeof(buff));
            if(r_res < 0) {
              close(sockfd);
              return 1;
            }
        }  

        if(packet_callback(ctx, buff, np)) {
            close(sockfd);
            return 0; 
        }

        // append to packet list
        if(np != NULL) {
            printf("NOT NULL ?!\n");
            np->next = cursor->next;
            cursor->next = np;
        }

        struct packet * tmp = cursor;
        cursor = cursor->next;
        bzero(buff, sizeof(buff));

        printf("freeing packet %x\n",tmp->payload[0]);
        free(tmp->payload);
        free(tmp);
        printf("freed packet\n");
    }

    close(sockfd);
    return 0;
}
