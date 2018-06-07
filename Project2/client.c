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

    /*
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) //establish a connection to the server
        error("ERROR connecting");

    printf("Please enter the message: ");
    memset(buffer, 0, 256);
    fgets(buffer, 255, stdin);  // read message

    n = write(sockfd, buffer, strlen(buffer));  // write to the socket
    if (n < 0)
         error("ERROR writing to socket");

    memset(buffer, 0, 256);
    n = read(sockfd, buffer, 255);  // read from the socket
    if (n < 0)
         error("ERROR reading from socket");
    printf("%s\n", buffer);  // print server's response

    close(sockfd);  // close socket
    */

    // retrieve filename
    char* file = argv[3];

    // setup file to receive data
    FILE* received_bytes;
    received_bytes = fopen("received.data", "a");
    if (received_bytes == NULL) {
        error("ERROR, unable to create a file to write to.");
    }

    // send SYN to begin establishing connection to server
    Packet request = {
        .seq_num = 0,
        .ack_num = 0,
        .payload_len = 0,
        .type = SYN
    };

    send_packet(sockfd, (struct sockaddr*)&serv_addr, serv_addr_len, request);
    printf("Sending packet %d SYN\n", request.seq_num);

    // receive packets from the server
    while (1) {
        // retrieve the packet
        Packet received = recv_packet(sockfd, (struct sockaddr*) &serv_addr, serv_addr_len);

        // print receiving message
        char* type = packet_type(received.type); 
        printf("Receiving packet %d%s\n", received.ack_num, type);

        // save contents of payload to file
        fwrite(received.payload, received.payload_len, 1, received_bytes);

        Packet* response = malloc(sizeof(Packet));

        // create the response packet
        if (received.type == SYN_ACK) {
            // finish 3-way handshake
            response->seq_num = received.seq_num+1;
            response->ack_num = received.ack_num;
            response->payload_len = sizeof(file);
            response->type = ACK;
            strcpy(response->payload, file);

            printf("seq: %d\nack: %d\n", response->seq_num, response->ack_num);
        } else if (received.type == FIN) {
            // accept connection termination
            response->seq_num = received.ack_num;
            response->ack_num = received.seq_num+1;
            response->payload_len = 0;
            response->type = FIN_ACK;
        } else if (received.type == ACK) {
            // terminate the connection
            break;
        } else {
            // send packets normally
            response->seq_num = received.ack_num;
            response->ack_num = received.seq_num + received.payload_len;
            response->payload_len = 0;
            response->type = NONE;
        }

        // send packet and print sending message
        send_packet(sockfd, (struct sockaddr*)&serv_addr, serv_addr_len, *response);
        type = packet_type(response->type); 
        printf("Sending packet %d%s\n", response->seq_num, type); 
        free(response);
    }

    // close file at the end
    fclose(received_bytes);

    return 0;
}