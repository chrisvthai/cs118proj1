#ifndef HELPER_H
#define HELPER_H

#define PACKET_SIZE 1024 // max packet size
#define PAYLOAD_SIZE (PACKET_SIZE - 2 * sizeof(int) - sizeof(enum Type)) // max packet size that can fit in the payload

#include <sys/socket.h>
#include <netdb.h> 

// establishing and taking down a connection
enum Type {NONE, SYN, SYN_ACK, ACK, FIN, FIN_ACK};

// packet struct that will be sent over TCP
typedef struct 
{
    int seq_num;
    int payload_len;
    enum Type type;
    char payload[PAYLOAD_SIZE];
} Packet;

void write_socket(Packet* packet, int sockfd, struct sockaddr_in* sock_addr, socklen_t sock_len);
void error(char *msg);

#endif HELPER_H