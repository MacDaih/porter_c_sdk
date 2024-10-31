#ifndef PORTER_SDK_H_   /* Include guard */
#define PORTER_SDK_H_

typedef enum {
    QOS_ZERO = 0x00;
    QOS_ONE = 0x08;
    QOS_TWO = 0x18;
} QOS ;

struct {
    char * addr;
    int port;

    char * client_id;
    char * client;
    char * client_pwd;

    uint16_t session_duration;
    uint16_t keep_alive;
    uint8_t qos; 

    _Bool connected;
} typedef client;

int init_client(client* c, QOS qos, uint16_t session_duration, uint16_t keep_alive);

void start(client* c, packet * p);

void client_send(client * c, char * topic, char * format, char * payload);
void client_recv(client * c, char topic[][]);
#endif // PORTER_SDK_H_
