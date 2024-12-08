#include "tcp_client.h"
#include "packet.h"
#include <stdio.h>

int dial_start(char * addr, int port, context ctx, struct packet * connect) {
    //free_list(connect);    
    struct packet* tmp;
    while (connect != NULL) {
        tmp = connect;
        connect = connect->next;
        printf("cmd : 0x%x\n", tmp->payload[0]);
    }
    return 0;
}
