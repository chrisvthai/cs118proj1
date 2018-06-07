/* A simple server in the internet domain using TCP
   The port number is passed as an argument
   This version runs forever, forking off a separate
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>  /* signal name macros, and the kill() prototype */

#include <math.h>
#include "helper.h"

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // create socket
    if (sockfd < 0)
        error("ERROR opening socket");
    memset((char *) &serv_addr, 0, sizeof(serv_addr));  // reset memory

    // fill in address info
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    socklen_t serv_addr_len = sizeof(serv_addr);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);  // 5 simultaneous connection at most

    // accept connections 
    // newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    // if (newsockfd < 0)
    //  error("ERROR on accept");

    // int n;
    // char buffer[256];

    // memset(buffer, 0, 256);  // reset memory

    // //read client's message
    // n = read(newsockfd, buffer, 255);
    // if (n < 0) error("ERROR reading from socket");
    // printf("Here is the message: %s\n", buffer);

    // //reply to client
    // n = write(newsockfd, "I got your message", 18);
    // if (n < 0) error("ERROR writing to socket");

    // close(newsockfd);  // close connection
    // close(sockfd);

    // assumption that the requested file is in the current directory

    char buf[PACKET_SIZE];
    char* file = 0;
    int num_packets = 0;
    FILE* open_this;
    char *file_buffer;

    // array of packets for easy access
    Packet** packet_list;

    while(1) {
        // retrieve the packet
        memset(buf, 0, PACKET_SIZE);
        recvfrom(sockfd, buf, PACKET_SIZE, 0, (struct sockaddr *) &serv_addr, &serv_addr_len);
        
        Packet* received = (Packet*) buf;

        // send the SYN ACK
        Packet response;
        if (received->type == SYN) {
            printf("Receiving packet %d SYN\n", received->seq_num);
            Packet response = {
                .seq_num = received->seq_num + received->payload_len+1,
                .payload_len = 0,
                .type = SYN_ACK,
                .payload = 0
            };
            write_socket(&response, sockfd, &serv_addr, sizeof(serv_addr));
            printf("Sending packet %d SYN ACK", response.seq_num);
            continue;
        } 
        // If the received packet is an ACK, then split the file into packets for transfer
        else if (received->type == ACK) {
            printf("Receiving packet %d ACK\n", received->seq_num);

            char* file;
            const char* rec_payload = received->payload;
            strncpy(file, rec_payload, received->payload_len);
            open_this = fopen(file, "r");

            //Get file length and read file into buffer
            fseek(open_this, 0, SEEK_END);
            long int size = ftell(open_this);
            fseek(open_this, 0, SEEK_SET);
            file_buffer = (char *) malloc(size);
            if (fread(file_buffer, size, 1, open_this) != size) {
                perror("Error reading from file");
                exit(1);
            }
            fclose(open_this);

            //Determine how many packets are needed to transmit the entire file
            num_packets = (int) (size/PAYLOAD_SIZE + 1);

            //Incrementally copy some of the buffer into a packet, then add the packet to a list of packets to be sent
            // FILE* write_to = fopen("output.data", "a");
            char payload[PAYLOAD_SIZE];
            int offset = 0;
            packet_list = malloc(ceil((double)size/PAYLOAD_SIZE) * sizeof(Packet*));
            
            for (int i = 0; i < num_packets; i++) {
                memcpy(payload, file_buffer+offset, PAYLOAD_SIZE);
                offset += PAYLOAD_SIZE;
                // fwrite(payload, PAYLOAD_SIZE, 1, write_to);

                // for the last packet, calculate the payload size manually
                int payload_len = PAYLOAD_SIZE;
                if (i == num_packets-1) {
                    payload_len = size - (PAYLOAD_SIZE * (num_packets-1));
                }

                Packet temp = {
                    .seq_num = 0,
                    .payload_len = payload_len,
                    .type = NONE
                };
                strcpy(temp.payload, payload);

                packet_list[i] = &temp;
            }

            // fclose(write_to);
            free(buf);
        }   

    return 0;
    }
}