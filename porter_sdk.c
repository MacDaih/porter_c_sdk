#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>


#include "porter_sdk.h"
#include "packet.h"
#include "tcp_client.h"

const char * CLIENT_ID = "PORTER_CLIENT_ID";
const char * CLIENT_PWD = "PORTER_CLIENT_PWD";
const char * CLIENT_USR = "PORTER_CLIENT_USER";
const char * SERVER_ADDR = "SERVER_ADDR";

char** client_subscriptions;

QOS parse_qos(int raw) {
    switch(raw) {
        case 2:
            return QOS_TWO;
        case 1:
            return QOS_ONE;
        default:
            return QOS_ZERO;
    }
}

int init_client(client * c, int qos, uint16_t session_duration, uint16_t keep_alive) {
    char * id = getenv(CLIENT_ID);
    if(!id) {
        printf("failed to read client id : CLIENT_ID not set");
        return 1;
    }
    c->client_id = id;

    char * usr = getenv(CLIENT_USR);
    if(!usr) {
        printf("failed to read client user name : USR not set");
        return 1;
    }
    c->client = usr;


    char * pwd = getenv(CLIENT_PWD);
    if(!pwd) {
        printf("failed to read client user password : PWD not set");
        return 1;
    }
    c->client_pwd = pwd;

    char * addr = getenv(SERVER_ADDR);
    if(!addr) {
        printf("failed to read server address : ADDR not set");
        return 1;
    }
    c->addr = addr;

    c->port = 8080;

    //TODO check keep_alive < session_duration
    c->keep_alive = keep_alive;
    c->session_duration = session_duration;
    c->qos = parse_qos(qos);

    return 0;
}


struct packet * init_connect(context ctx) {
    struct packet * p = new_packet();

    property * conn_props = calloc(8, sizeof(property));
    if(ctx.user && ctx.pwd) {
        property auth = {.key = 0x15, .prop_value.enc_str = "Password", .prop_type = STRING};
        conn_props[0] = auth; 
    }
    make_connect(ctx,p, conn_props);

    free(conn_props);
    return p;
} 

struct packet * init_publish(client * c,char * topic, char * format, char * payload) {
    struct packet * pub = new_packet();
    // make properties
    property * props = calloc(7,sizeof(property));
    if(format) {
        // format indicator
        property fi = { 
            .key = 0x01,
            .prop_value.byte = 0x01,
            .prop_type = UINT8,
        }; 
        props[0] = fi;
        // content type
        property ct = { 
            .key = 0x03,
            .prop_value.enc_str = format,
            .prop_type = STRING,
        }; 
        props[1] = ct;
    }

    make_publish(pub, topic, payload, props);
    free(props);
    return pub;
}

struct packet * init_subcribe(char * topics[]) {
    // TODO make properties
    struct packet * sub = new_packet();
    make_subscribe(sub, topics);
    return sub;
}

struct packet * init_disc() {
    struct packet * disconn = new_packet();
    make_disconnect(disconn);
    return disconn;
}

int client_send(client * c, char * topic, char * format, char * payload) {
    // TODO create method for context init
    context ctx;
    ctx.user_flag = 1;
    ctx.pwd_flag = 1;
    ctx.will_retain = 0;
    ctx.will_qos = 0;
    ctx.will_flag = 0;
    ctx.clean_start = 0;

    ctx.cid = c->client_id;
    ctx.user = c->client;
    ctx.pwd = c->client_pwd; 
    ctx.keep_alive = c->keep_alive;
    //
    
    struct packet * conn = init_connect(ctx);

    struct packet * pub = init_publish(c, topic, format, payload);

    unsigned char disc[4] = {0x0e, 0x01, 0x00, 0x00};
    struct packet * disconn = new_from_payload(disc);

    pub->next = disconn;
    conn->next = pub;

    if(dial_start(c->addr, c->port, ctx, conn)) {
        printf("failed to dial to server %s\n", strerror(errno));    
        return 1;
    }
    
    return 0;
}

int client_recv(client * c, char * topics[]) {

    // TODO create method for context init
    context ctx;
    ctx.user_flag = 1;
    ctx.pwd_flag = 1;
    ctx.will_retain = 0;
    ctx.will_qos = 0;
    ctx.will_flag = 0;
    ctx.clean_start = 0;

    ctx.cid = c->client_id;
    ctx.user = c->client;
    ctx.pwd = c->client_pwd; 
    ctx.keep_alive = c->keep_alive;
    //

    struct packet * conn = init_connect(ctx);
    // TODO verify if client has already subscribed to topics from args
    // then add to message list
    struct packet * sub = init_subcribe(topics);
    conn->next = sub;

    // TODO solve delayed disconnect
    if(dial_start(c->addr, c->port, ctx, conn)) {
        printf("failed to dial to server %s\n", strerror(errno));    
        return 1;
    }

    return 0;
}
