#ifndef PACKET_H_   /* Include guard */
#define PACKET_H_

#include <stdint.h>
//const unsigned char MQTT[6] = {0x00,0x04,0x4D,0x51,0x54,0x54};

struct packet {
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
void make_publish(context ctx, struct packet * pkt, char * topic,char * payload, property * props);

int publish_callback(struct packet * p, unsigned char * payload);
#endif // PACKET_H_
