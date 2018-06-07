#include "helper.h"

// prints error message and exits form program with error
void error(char *msg) {
    perror(msg);
    exit(1);
}

// send a packet
void send_packet(int sockfd, struct sockaddr *src_addr, socklen_t addrlen, Packet to_send) {
    Bytes b_send = {
        .my_packet = to_send
    };
    ssize_t n = sendto(sockfd, b_send.bytes, PACKET_SIZE, 0, src_addr, addrlen);
    if (n < 0) {
        error("ERROR, problem with sending data to socket.\n");
    }
}

// retrieve a packet
Packet recv_packet(int sockfd, struct sockaddr *src_addr, socklen_t addrlen) {
    Bytes to_get;
    ssize_t n = recvfrom(sockfd, to_get.bytes, PACKET_SIZE, 0, src_addr, &addrlen);
    if (n < 0) {
        error("ERROR, couldn't retrieve packet");
    }
    Packet new_packet = to_get.my_packet;

    return new_packet;
}

// returns the type of response
char* packet_type(enum Type p_type) {
    char* temp;
    switch (p_type) {
        case(SYN):
            temp = " SYN";
            break;
        case(SYN_ACK):
            temp = " SYN";
            break;
        case(FIN):
            temp = " FIN";
            break;
        case(FIN_ACK):
            temp = " FIN";
            break;
        default:
            temp = "";
    }
    return temp;
}

// Creates a packet
Packet packet_gen(int seq, int ack, int p_len, enum Type t, const char* data) {
    Packet temp = {
        .seq_num = seq,
        .ack_num = ack, 
        .payload_len = p_len,
        .type = t
    };
    if (data != NULL) {
        strncpy(temp.payload, data, p_len);
    }

    return temp;
}