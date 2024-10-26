#include <stdlib.h>
#include <stdio.h>

#include "porter_sdk.h"
#include "packet.h"
#include "tcp_client.h"

const char * CLIENT_ID = "CLIENT_ID";
const char * CLIENT_PWD = "PWD";
const char * CLIENT_USR = "USER";
const char * SERVER_ADDR = "SERVER_ADDR";


int init_client(client * c, QOS qos, uint16_t session_duration, uint16_t keep_alive) {
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
    c->qos = qos;

    return 0;
}

// send
// receive
packet * build_connect(client c) {
    struct packet * p = new_packet();

    context ctx;
    if(c.client_id) ctx.user_flag = 1;
    if(c.client_pwd) ctx.pwd_flag = 1;
    
    ctx.will_qos = c.qos; 

    ctx.will_retain = 0;
    ctx.will_flag = 0;
    ctx.clean_start = 0;

    ctx.cid = c.client_id;
    ctx.user = c.client;
    ctx.pwd = c.client_pwd; 
    ctx.keep_alive = c.keep_alive;

    property * conn_props = calloc(8, sizeof(property));
    if(ctx.user && ctx.pwd) {
        property auth = {.key = 0x15, .prop_value.enc_str = "Password", .prop_type = STRING};
        conn_props[0] = auth; 
    }
    make_connect(ctx,p, conn_props);

    free(conn_props);

    return p;
}

int subscribe(client c, char ** topic) {
    struct packet * connect = new_packet();
    struct packet * subscribe = new_packet();



    uint16_t interval = c.session_duration / c.keep_alive;
    for(uint16_t i = 0; i < interval; i++) {
        struct packet * ping = new_packet();
        
    }

    struct packet * disc = new_packet();
    return 0;
}

int publish(client c, char * topic, char * format, char * payload) {
    struct packet * p = new_packet();
    struct packet * pub = new_packet();
    struct packet * disc = new_packet();

    context ctx;
    ctx.user_flag = 1;
    ctx.pwd_flag = 1;
    ctx.will_retain = 0;
    ctx.will_qos = 0;
    ctx.will_flag = 0;
    ctx.clean_start = 0;

    ctx.cid = c.client_id;
    ctx.user = c.client;
    ctx.pwd = c.client_pwd; 
    ctx.keep_alive = 10;

    property * conn_props = calloc(8, sizeof(property));
    if(ctx.user && ctx.pwd) {
        property auth = {.key = 0x15, .prop_value.enc_str = "Password", .prop_type = STRING};
        conn_props[0] = auth; 
    }
    make_connect(ctx,p, conn_props);

    free(conn_props);

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

    make_publish(ctx, pub, topic, payload, props);
    free(props);

    pub->next = disc;
    p->next = pub;

    int res = dial_start(c.addr, c.port, p, packet_callback);    
    if(res != 0) printf("failed to publish message\n");

    free_packet(p);
    free_packet(pub);
    free_packet(disc);

    return res;
}

int subscribe(client c, char** topics) {
   return 0;
} 
