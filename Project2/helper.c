#include "helper.h"

// prints error message and exits form program with error
void error(char *msg)
{
    perror(msg);
    exit(1);
}

// write the packet to the socket to be sent over UDP
void write_socket(Packet* packet, int sockfd, struct sockaddr_in* sock_addr, socklen_t sock_len) {
    char buf[PACKET_SIZE];
    memset(buf, 0, PACKET_SIZE);

    char* my_packet = (char*) packet;
    strcpy(buf, my_packet);

    char* n = sendto(sockfd, buf, PACKET_SIZE, 0, sock_addr, sock_len);
    if (n < 0) {
        error("ERROR, problem with sending data to socket.\n");
    }
}