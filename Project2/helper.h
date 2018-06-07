#ifndef HELPER_H
#define HELPER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#define PACKET_SIZE 1024 // max packet size
#define PAYLOAD_SIZE (PACKET_SIZE - 3 * sizeof(int) - sizeof(enum Type)) // max packet size that can fit in the payload

// establishing and taking down a connection
enum Type {NONE, SYN, SYN_ACK, ACK, FIN, FIN_ACK};

// packet struct that will be sent over TCP
typedef struct {
    int seq_num;
    int ack_num;
    int payload_len;
    enum Type type;
    char payload[PAYLOAD_SIZE];
} Packet;

// union to convert a packet into bytes
typedef union {
    Packet my_packet;
    char bytes[sizeof(Packet)];
} Bytes;

void error(char *msg);
void send_packet(int sockfd, struct sockaddr *src_addr, socklen_t addrlen, Packet to_send);
Packet recv_packet(int sockfd, struct sockaddr *src_addr, socklen_t addrlen);
char* packet_type(enum Type type);
Packet packet_gen(int seq, int ack, int p_len, enum Type t, const char* data);

#endif // HELPER_H