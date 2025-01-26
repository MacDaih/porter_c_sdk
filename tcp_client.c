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
    
    int sockfd;
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
        return 1;
    }

    unsigned char buff[MAX];

    struct pollfd fd;
    fd.fd = sockfd;
    fd.events = POLLIN;

    int code = 0;
    while(cursor) {
        printf("cursor packet %x\n",cursor->payload[0]);
        int m_res = write(sockfd, cursor->payload, cursor->len);
        if(m_res < 0) {
            code = 1;
            break;
        }
         
        struct packet * np = NULL;
        if(poll(&fd, 1, (int)(ctx.keep_alive * 1000)) > 0) {
          int r_res = recv(sockfd,buff,sizeof(buff), 0);
          if(r_res < 0) {
              code = 1;
              break;
          }
        } else if (ctx.keep_alive > 0) {
            struct packet * ping = new_packet();
            make_ping(ping);

            bzero(buff, sizeof(buff));
            if(write(sockfd, ping->payload, ping->len)) {
                printf("failed to write ping to server %s\n", strerror(errno));    
                code = 1;
                break;
            }

            free(ping);
            int r_res = recv(sockfd, buff, sizeof(buff), 0);
            if(r_res < 0) {
                printf("failed to read ping response from server %s\n", strerror(errno));    
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

        struct packet * tmp = cursor;
        cursor = cursor->next;
        bzero(buff, sizeof(buff));
        

        free(tmp->payload);
        free(tmp);
    }

    close(sockfd);
    return code;
}
