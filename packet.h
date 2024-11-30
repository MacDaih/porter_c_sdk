#ifndef PACKET_H_   /* Include guard */
#define PACKET_H_

#include <stdint.h>
//const unsigned char MQTT[6] = {0x00,0x04,0x4D,0x51,0x54,0x54};

// Packet Command Codes
//

#define CONNECT_CMD  0x10
#define CONNACK_CMD  0x20
#define PUBLISH_CMD  0x030
#define PUBACK_CMD  0x40
#define PUBREC_CMD  0x50
#define PUBREL_CMD  0x60
#define PUBCOMP_CMD 0x70
#define SUBSCRIBE_CMD  0x80
#define SUBACK_CMD  0x90
#define UNSUBSCRIBE_CMD  0xa0
#define UNSUBACK_CMD  0xb0
#define PINGREQ_CMD  0xc0
#define PINGRESP_CMD 0x0d
#define DISCONNECT_CMD  0xe0
#define AUTH_CMD  0xf0

typedef enum {
    QOS_ZERO = 0x00,
    QOS_ONE = 0x08,
    QOS_TWO = 0x18,
} QOS ;

struct packet {
    uint16_t id;
    size_t cursor;
    size_t len;
    uint8_t type;
    unsigned char * payload;
    struct packet * next;
};

void write_byte(uint8_t byte, struct packet * pkt);

struct {
    unsigned user_flag:1;
    unsigned pwd_flag:1;
    unsigned will_retain:1;
    unsigned will_qos:2;
    unsigned will_flag:1;
    unsigned clean_start: 1;

    uint16_t keep_alive;

    char *user;
    char *pwd;
    char *cid;
} typedef context;

union {
    uint32_t varint;
    uint32_t uint32;
    uint16_t uint16;
    uint8_t byte;
    char * enc_str;
} typedef prop_value;

enum prop_type {
    VARINT,
    UINT32,
    UINT16,
    UINT8,
    STRING,
};

struct {
    uint8_t key;
    enum prop_type prop_type;
    prop_value prop_value;
} typedef property;

enum packet_type {
    connect_cmd = 0x10,
    connack_cmd = 0x20,
    publish_cmd = 0x30,
    puback_cmd = 0x40,
    pubrec_cmd = 0x50,
    pubrel_cmd = 0x60,
    pubcomp_cmd = 0x70,
    subscribe_cmd = 0x80,
    suback_cmd = 0x90,
    unsubscribe_cmd = 0xA0,
    unsuback_cmd = 0xB0,
    pingreq_cmd = 0xC0,
    pingresp_cmd = 0xD0,
    disconnect_cmd = 0xE0,
    auth_cmd = 0xF0,
};

struct packet * new_packet();
void free_packet(struct packet * p);

void encode_str(char * raw, struct packet * pkt);
void encode_int(uint16_t in, struct packet * pkt);
int read_varint(struct packet * pkt, uint32_t * word);
size_t eval_bytes(uint32_t value);
void encode_varint(uint32_t x, struct packet * pkt);
void write_remaining_len(struct packet * pkt);
void write_fix_header(enum packet_type ptype, struct packet * pkt);

void make_connect(context ctx, struct packet * pkt, property * props);
void make_disconnect(struct packet * pkt);
void make_publish(struct packet * pkt, char * topic,char * payload, property * props);
void make_subscribe(struct packet * pkt, char * topics[]);
void make_ping(struct packet * pkt);

int packet_callback(context ctx, unsigned char * payload, struct packet * receiver);

void free_list(struct packet * pkt);
#endif // PACKET_H_
