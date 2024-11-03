#ifndef PORTER_SDK_H_   /* Include guard */
#define PORTER_SDK_H_

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

int init_client(client* c, int qos, uint16_t session_duration, uint16_t keep_alive);

void client_send(client * c, char * topic, char * format, char * payload);
void client_recv(client * c, char * topic[]);
#endif // PORTER_SDK_H_
