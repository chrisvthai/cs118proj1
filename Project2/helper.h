#ifndef HELPER_H
#define HELPER_H

#define PACKET_SIZE 1024 // max packet size
#define PAYLOAD_SIZE (PACKET_SIZE - sizeof(int) - sizeof(enum Type)) // max packet size that can fit in the payload

enum Type{NONE, SYN, SYN_ACK, ACK, FIN, FIN_ACK}

struct Packet
{
    int seq_num;
    enum Type
    char payload[PAYLOAD_SIZE];
}

void error(char *msg);

#endif HELPER_H