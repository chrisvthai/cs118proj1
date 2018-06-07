#include "helper.h"

// prints error message and exits form program with error
void error(char *msg)
{
    perror(msg);
    exit(1);
}

// write the packet to the socket to be sent over UDP
void write_socket(Packet packet, int sockfd, struct sockaddr_in* sock_addr, socklen_t sock_len) {
    char buf[PACKET_SIZE];
    memset(buf, 0, PACKET_SIZE);
    
}