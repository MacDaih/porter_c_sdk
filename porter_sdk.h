#ifndef PORTER_SDK_H_   /* Include guard */
#define PORTER_SDK_H_

struct {
    char * addr;
    int port;

    char * client_id;
    char * client;
    char * client_pwd;
} typedef client;

int init_client(client*);

int publish(client c, char * topic, char * format, char * payload);
#endif // PORTER_SDK_H_
