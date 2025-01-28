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
    
    struct sockaddr_in servaddr, cli;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("failed to init socket %s\n", strerror(errno));    
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

    int code = 0;
    while(cursor) {
        printf("sending 0x%2x, len -> %d\n", cursor->payload[0], cursor->len);
        int m_res = write(sockfd, cursor->payload, cursor->len);
        if(m_res < 0) {
            printf("failed to write to server %s\n", strerror(errno));    
            code = 1;
            break;
        }
        
        // refactor
        struct packet * np = NULL;
        if(ctx.keep_alive > 0) {
            int pres = poll(&fd, 1, (int)(ctx.keep_alive * 1000));
            if(pres < 0) {
                printf("failed to poll response from server %s\n", strerror(errno));    
                close(sockfd);
                return 1;
            }

             if(pres == 0) {
                    struct packet * ping = new_packet();
                    make_ping(ping);

                    bzero(buff, sizeof(buff));
                    if(write(sockfd, ping->payload, ping->len)) {
                        printf("failed to write ping to server %s\n", strerror(errno));    
                        code = 1;
                        break;
                    }

                    free(ping);
                    int r_res = read(sockfd, buff, sizeof(buff));
                    if(r_res < 0) {
                        printf("failed to read ping response from server %s\n", strerror(errno));    
                        code = 1;
                        break;
                    }
             }

             int r_res = read(sockfd,buff,sizeof(buff));
             if(r_res < 0) {
                printf("failed to read from server %s\n", strerror(errno));    
                code = 1;
                break;
             }   
        } else {
          int r_res = read(sockfd,buff,sizeof(buff));
          if(r_res < 0) {
              printf("failed to read from server %s\n", strerror(errno));    
              code = 1;
              break;
          }

          printf("debug no ping read %d", r_res);
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
