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

#include "packet.h"

#define MAX 1024

int dial_start(
        char * addr,
        int port,
        context ctx,
        struct packet * p
) {
    int sockfd;
    struct packet * cursor = p;
    
    struct sockaddr_in servaddr, cli;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("failed to init socket %s\n", strerror(errno));    
        return 1;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(addr);
    servaddr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("failed to connect to server %s\n", strerror(errno));    
        return 1;
    }

    unsigned char buff[MAX];

    int code = 0;

    //if(ctx.keep_alive > 0) {
    struct timeval tv;
    tv.tv_sec = ctx.keep_alive;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval*)&tv, sizeof(struct timeval));
    //}

    while(cursor) {
        int m_res = send(sockfd, cursor->payload, cursor->len, 0);
        if(m_res < 0) {
            printf("failed to write to server %s\n", strerror(errno));    
            code = 1;
            break;
        }
        
         struct packet * np = NULL;
    
         int r_res = recv(sockfd,buff,sizeof(buff),0);
         if(r_res < 0) {
            if(errno == EWOULDBLOCK) {
                struct packet * ping = new_packet();
                make_ping(ping);

                bzero(buff, sizeof(buff));
                send(sockfd, ping->payload, ping->len, 0);

                free(ping);
                recv(sockfd, buff, sizeof(buff), 0);
                bzero(buff, sizeof(buff));
            } else {
                printf("failed to read from server %s\n", strerror(errno));    
                code = 1;
                break;
            }
        }   

        int cres = packet_callback(ctx, buff, np);
        if(cres > 0) {
            code = cres;
            break;
        }

        // append to packet list
        if(np != NULL) {
            np->next = cursor->next;
            cursor->next = np;
        }

        cursor = cursor->next;
        bzero(buff, sizeof(buff));
    }
    close(sockfd);
    return code;
}
