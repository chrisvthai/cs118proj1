/*
 A simple client in the internet domain using TCP
 Usage: ./client hostname port (./client 192.168.0.151 10000)
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>
#include <sys/types.h>
#include "helper.h"

int main(int argc, char *argv[])
{
    int sockfd;  // socket descriptor
    int portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;  // contains tons of information, including the server's IP address

    char buffer[256];
    if (argc < 4) {
       fprintf(stderr, "Usage: ./%s hostname portno filename\n", argv[0]);
       exit(1);
    }

    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // create a new socket
    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(argv[1]);  // takes a string like "www.yahoo.com", and returns a struct hostent which contains information, as IP address, address type, the length of the addresses...
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; //initialize server's address
    bcopy((char *)server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    socklen_t serv_addr_len = sizeof(serv_addr);

    // retrieve filename
    char* file = argv[3];

    // setup file to receive data
    FILE* received_bytes;
    received_bytes = fopen("received.data", "a");
    if (received_bytes == NULL) {
        error("ERROR, unable to create a file to write to.");
    }

    // send SYN to begin establishing connection to server
    Packet request = packet_gen(0, 0, 0, 0, SYN, NULL);
    send_packet(sockfd, (struct sockaddr*)&serv_addr, serv_addr_len, request);
    printf("Sending packet %d SYN\n", request.seq_num);

    // setup select
    fd_set sockets;
    int response_flag = 0;
    int gotfile_flag = 0;

    // receive packets from the server
    while (1) {
        // setup check for connection timeout
        FD_ZERO(&sockets);
        FD_SET(sockfd, &sockets);

        struct timeval connection_timeout = {1, 0};
        int ret = select(sockfd+1, &sockets, NULL, NULL, &connection_timeout);
        if (ret < 0) {
            fclose(received_bytes);
            error("ERROR, select failed");
        } else if (ret == 0) {
            // timeout expired
            if (gotfile_flag) {
                // finished getting the file and then the socket failed
                // technically don't need to maintain the connection
                error("ERROR, retrieved file, but unable to terminate connection");
            } else if (!response_flag) {
                // client only sends one packet with data so if request hasn't been confirmed retry connection
                send_packet(sockfd, (struct sockaddr*)&serv_addr, serv_addr_len, request);
                printf("Sending packet Retransmission SYN\n");
                continue;
            } 
        }

        // socket is readable
        if (FD_ISSET(sockfd, &sockets)) {
            // retrieve the packet
            Packet received = recv_packet(sockfd, (struct sockaddr*) &serv_addr, serv_addr_len);
            char* type = packet_type(received.type); 
            printf("Receiving packet %d%s\n", received.ack_num, type);

            // save contents of payload to file
            fwrite(received.payload, received.payload_len, 1, received_bytes);

            Packet response;
            // create the response packet
            if (received.type == SYN_ACK) {
                // finish 3-way handshake
                response = packet_gen(1, 1, sizeof(file), 0, ACK, file);
                response_flag = 1;
            } else if (received.type == FIN) {
                // accept connection termination
                response = packet_gen(received.ack_num, received.seq_num + received.payload_len+1, 0, 0, FIN_ACK, NULL);
                gotfile_flag = 1;
            } else if (received.type == ACK) {
                // terminate the connection
                break;
            } else {
                // send packets normally
                response = packet_gen(received.ack_num, received.seq_num + received.payload_len, 0, 0, ACK, NULL);
            }

            // send packet and print sending message
            send_packet(sockfd, (struct sockaddr*)&serv_addr, serv_addr_len, response);
            type = packet_type(response.type); 
            
            // if the current packet is from before the current window
            // if (received.offset < window_base) {
            //     printf("Sending packet %d Retransmission %s\n");
            // } else {
                printf("Sending packet %d%s\n", response.seq_num, type); 
            // }
        }
    }

    // close file at the end
    fclose(received_bytes);

    return 0;
}
