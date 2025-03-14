#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "tcp_client.h"


const uint8_t USER_FLAG = 0x80;
const uint8_t PWD_FLAG = 0x40;
const uint8_t WILL_RETAIN = 0x20;
const uint16_t WILL_QOS = 0x0000;
const uint8_t WILL_FLAG = 0x00;

const size_t MAX_TOPICS = 10;

struct params {
    char * user;
    char *pwd;
    uint8_t qos;
};

void write_byte(uint8_t byte, struct packet * pkt) {
    pkt->payload[pkt->cursor] = byte;
    pkt->cursor++;
}

void write_byte_buffer(uint8_t byte, uint8_t *buff, int * cursor) {
    int c = *cursor;
    buff[c] = byte;
    (*cursor)++;
}

unsigned char read_byte(struct packet * pkt) {
    unsigned char byte = pkt->payload[pkt->cursor];
    pkt->cursor++;
    return byte;
}

void write_uint16(uint16_t val, struct packet * pkt) {
    write_byte(val & 0xff00 >> 8, pkt);
    write_byte(val & 0x00ff, pkt);
}

void write_uint16_buffer(uint16_t val, uint8_t * buff, int * cursor) {
    write_byte_buffer(val & 0xff00 >> 8, buff, cursor);
    write_byte_buffer(val & 0x00ff, buff, cursor);
}

void write_uint32_buffer(uint32_t val, uint8_t * buff, int * cursor) {
    write_byte_buffer((val & 0xff000000) >> 24, buff, cursor);
    write_byte_buffer((val & 0x00ff0000) >> 16, buff, cursor);
    write_byte_buffer((val & 0x0000ff00) >> 8, buff, cursor);
    write_byte_buffer(val & 0x000000ff, buff, cursor);
}

void encode_str(char * raw, struct packet * pkt) {
    size_t base = strlen(raw);
    
    uint8_t msb = (base & 0xff00) >> 8;
    uint8_t lsb = base & 0x00ff;


    write_byte(msb, pkt);
    write_byte(lsb, pkt);

    for(size_t i = 0; i < base; i++) {
        write_byte((uint8_t)raw[i], pkt);       
    }
}

void encode_str_buffer(char * raw, uint8_t * buff, int *cursor) {
    size_t base = strlen(raw);
    
    uint8_t msb = (base & 0xff00) >> 8;
    uint8_t lsb = base & 0x00ff;

    write_byte_buffer(msb, buff, cursor);
    write_byte_buffer(lsb, buff, cursor);

    for(size_t i = 0; i < base; i++) {
        write_byte_buffer((uint8_t)raw[i], buff, cursor);       
    }
}

void encode_int(uint16_t in, struct packet * pkt) {
    write_byte( (uint8_t)((in & 0xff00) >> 8), pkt);
    write_byte( (uint8_t)(in & 0x00ff), pkt);
}

int read_varint(struct packet * pkt, uint32_t * word) {
    int i;
	uint8_t byte;
	unsigned int remaining_mult = 1;
	uint32_t lword = 0;
	uint8_t lbytes = 0;

	for(i=0; i<4; i++){
		if(pkt->cursor < pkt->len){
			lbytes++;
			byte = pkt->payload[pkt->cursor];
			lword += (byte & 127) * remaining_mult;
			remaining_mult *= 128;
			pkt->cursor++;
			if((byte & 128) == 0){
				if(lbytes > 1 && byte == 0){
					return 1;
				}else{
					*word = lword;
					return 0;
				}
			}
		}else{
            return 1;
			//return MOSQ_ERR_MALFORMED_PACKET;
		}
	}
    return 1;
}

size_t eval_bytes(uint32_t value) {
    if (value < 128) { 
        return 1; 
    } else if(value < 16384) {
        return 2; 
    } else if (value < 2097152) {
        return 3; 
    } else if (value < 268435456) {
        return 4;
    } else { 
        return 5;
    }
}

void encode_varint(uint32_t x, struct packet * pkt) {
    if(x == 0) {
        write_byte(0,pkt);
        return;
    }

    uint8_t encodedByte;
    while(x > 0) {
        encodedByte = x % 128; 
        x = x / 128;
        if (x > 0)
            encodedByte |= 128;
        write_byte(encodedByte, pkt);
    }
}

void encode_varint_buffer(uint32_t x, uint8_t *buff,int * cursor) {
    if(x == 0) {
        write_byte_buffer(0,buff, cursor);
        return;
    }

    uint8_t encodedByte;
    while(x > 0) {
        encodedByte = x % 128; 
        x = x / 128;
        if (x > 0)
            encodedByte |= 128;
        write_byte_buffer(encodedByte, buff, cursor);
    }
}

void write_remaining_len(struct packet * pkt) {
    pkt->len = (pkt->cursor + 1) + 1;
}

void write_fix_header(enum packet_type ptype, struct packet * pkt) {
    write_byte((uint8_t)ptype, pkt);
}

uint8_t * write_properties(property props[], int prop_len, int * props_size) {

    uint8_t *enc = (uint8_t *) malloc(256 *sizeof(uint8_t));

    size_t final = 0;
    int cursor = 0;
    for(int i = 0; i < prop_len; i++) {
        property prop = props[i];
        if(prop.key == 0x00) break; 
        enc[cursor] = prop.key;
        cursor++;
        final++;
        switch(prop.prop_type) {
            case VARINT:
                encode_varint_buffer(prop.prop_value.uint32, enc, &cursor);
                final += eval_bytes(prop.prop_value.uint32);
                break;
            case UINT32:
                write_uint32_buffer(prop.prop_value.varint, enc, &cursor);
                final += 4;
                break;
            case UINT16:
                write_uint16_buffer(prop.prop_value.uint16, enc, &cursor);
                final += 2;
                break;
            case UINT8:
                write_byte_buffer(prop.prop_value.byte, enc, &cursor);
                final += 1;
                break;
            case STRING:
                encode_str_buffer(prop.prop_value.enc_str, enc, &cursor);
                final += strlen(prop.prop_value.enc_str) + 2;
                break;
            default:
                break;
        }
    }
    
    uint8_t *res;
    res = (uint8_t *) realloc(enc, final);
    *props_size = final;
    return res;
}

void make_connect(context ctx, struct packet * pkt, property props[8]) {
    assert(props);
    write_fix_header(connect_cmd, pkt);

    int prop_size = 0;
    uint8_t * prop_buff = write_properties(props, 8, &prop_size);
    size_t rem_len = eval_bytes(prop_size) + prop_size;

    if(ctx.user) 
        rem_len += strlen(ctx.user) + 2;

    if(ctx.pwd) 
        rem_len += strlen(ctx.pwd) + 2;

    if(ctx.cid) 
        rem_len += strlen(ctx.cid) + 2;

    rem_len += eval_bytes(rem_len) + 10;
    encode_varint(rem_len, pkt);

    // 4 + 2
    encode_str("MQTT", pkt);
    // Version 5
    // + 1
    write_byte(0x05, pkt);

    // Write flags
    // + 1 
	uint8_t flags = (ctx.user_flag << 7) | 
            (ctx.pwd_flag << 6) |
            (ctx.will_retain << 5) |
            (ctx.will_qos << 3) |
            (ctx.will_flag << 2) |
            (ctx.clean_start << 1);


    write_byte(flags, pkt);

    // + 2
    write_uint16(ctx.keep_alive, pkt);
    //

    encode_varint(prop_size, pkt);
    for(int i = 0; i < prop_size; i++)
        write_byte(prop_buff[i], pkt);
   
    // TODO will message
    encode_str(ctx.cid, pkt);
    encode_str(ctx.user, pkt);
    encode_str(ctx.pwd, pkt);
   
    pkt->len = 1 + rem_len; 
    pkt->cursor = 0;
} 

void make_ping(struct packet * pkt) {
    write_byte(pingreq_cmd, pkt);
    encode_varint(0, pkt);
    write_remaining_len(pkt);
    pkt->cursor = 0;
}

void make_publish(
    struct packet * pkt,
    char * topic,
    char * payload,
    property props[7]
) {
    write_byte(publish_cmd ^ (0 << 1), pkt);
    
    size_t rem_length = (strlen(topic) + 2) + strlen(payload) + 2;

    // write_uint16(0, pkt);
    // topic name
    int prop_size = 0; 
    uint8_t * prop_buff = write_properties(props, 7, &prop_size);

    rem_length += eval_bytes(prop_size) + prop_size;

    encode_varint(rem_length, pkt);

    encode_str(topic, pkt);
    // FIXME QoS 0 -> no packet id
    // write_uint16(0, pkt);

    // properties
    // property length
    encode_varint(prop_size, pkt);

    for(int i = 0; i < prop_size; i++) 
        write_byte(prop_buff[i], pkt);
    
    // write payload
    encode_str(payload, pkt);

    pkt->len = rem_length + 1 + eval_bytes(rem_length);
    printf("packet length : %d\n", pkt->len);
    pkt->cursor = 0;
}

void make_subscribe(
    struct packet * pkt,
    char * topics[]
    //property props[3]
) {
    
	write_byte(subscribe_cmd, pkt);

    struct packet tmp = {0};
    write_uint16(pkt->id, &tmp);

    // TODO props
    encode_varint(0, &tmp);
    
    char *ntopics[MAX_TOPICS];
    memcpy(ntopics, topics, MAX_TOPICS);
    for(int i = 0; i < (int)(MAX_TOPICS); i++) {
        encode_str(ntopics[i], &tmp);

        // TODO handle subscription options
        write_byte(0, &tmp);
    }

    int len = (int)(tmp.cursor) + 1;
    for(int c = 0; c < len; c++)
        write_byte(tmp.payload[c], pkt);

    write_remaining_len(pkt);
    pkt->cursor = 0;
}

//void make_puback(
//    context ctx, 
//    struct packet * pkt,
//    char * topic,
//    char * payload,
//    property props[2]
//) {}

void make_disconnect(struct packet * pkt) {
    write_byte(0xe0, pkt);
    encode_varint(1, pkt);

    //disconnect reason code
    write_byte(0x00, pkt);
    // no props for now
    encode_varint(0, pkt);

    write_remaining_len(pkt);
    pkt->cursor = 0;
}

void free_packet(struct packet * p) {
    free(p->payload);
    free(p);
}

struct packet * new_packet() {
    unsigned char * payload = (unsigned char *)malloc(1024 * sizeof(unsigned char));;

    struct packet * p = (struct packet*) malloc(sizeof(struct packet));

    p->payload = payload;
    p->cursor = 0;
    p->next = NULL;

    return p;
}

struct packet * new_from_payload(unsigned char * raw) {
    struct packet * p = (struct packet*) malloc(sizeof(struct packet));
    p->payload = raw;
    p->cursor = 0;
    return p;
}


int packet_callback(context ctx, unsigned char * payload, struct packet * receiver) {
    unsigned char cmd = payload[0];

    switch(cmd) {
        case CONNACK_CMD:
            return 0;
        case SUBACK_CMD:
            return 0;
        case PUBLISH_CMD:
            if(ctx.will_qos == QOS_ONE) // TODO handle puback depending on QoS
            return 0;
        case DISCONNECT_CMD:
            return 1;
        default:
            return 0;
    }
    return 0;
}


void free_list(struct packet * p) {
    while (p != NULL) {
        struct packet * tmp = p;
        p = p->next;
        free(tmp->payload);
        free(tmp);
    }
}
