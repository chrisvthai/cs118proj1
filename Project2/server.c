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

#include <sys/time.h>
#include <sys/types.h>
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

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    socklen_t cli_addr_len = sizeof(cli_addr);

    listen(sockfd, 5);  // 5 simultaneous connection at most

    // assumption that the requested file is in the current directory

    // array of packets for easy access
    Packet** packet_list = 0;
    int num_packets = 0;
    int curr_packet = 0;
    int fin_flag = 1;
    long int size; // size of file

    while(fin_flag) {
        // retrieve the packet
        Packet received = recv_packet(sockfd, (struct sockaddr*) &cli_addr, cli_addr_len);

        // print receiving message
        char* type = packet_type(received.type); 
        printf("Receiving packet %d%s\n", received.ack_num, type);

        Packet response;

        // create the SYN ACK
        if (received.type == SYN) {
            response = packet_gen(0, 1, 0, 0, SYN_ACK, NULL);
        } else if (received.type == ACK) {
            // If the received packet is an ACK, then split the file into packets for transfer
            char file[PAYLOAD_SIZE];
            memset(file, 0, PAYLOAD_SIZE);
            const char* rec_payload = received.payload;
            strncpy(file, rec_payload, received.payload_len);
            FILE* open_this = fopen(file, "r");

            //Get file length and read file into buffer
            fseek(open_this, 0, SEEK_END);
            size = ftell(open_this);
            fseek(open_this, 0, SEEK_SET);
            char* file_buffer = malloc(sizeof(char) * size);
            if (fread(file_buffer, size, 1, open_this) != 1) {
                perror("Error reading from file");
                exit(1);
            }
            fclose(open_this);

            //Determine how many packets are needed to transmit the entire file
            num_packets = (ceil((double)size/PAYLOAD_SIZE));

            //Incrementally copy some of the buffer into a packet, then add the packet to a list of packets to be sent
            char payload[PAYLOAD_SIZE];
            int offset = 0;
            packet_list = malloc(num_packets * sizeof(Packet*));
            
            for (int i = 0; i < num_packets; i++) {
                memcpy(payload, &file_buffer[offset], PAYLOAD_SIZE);
                
                // for the last packet, calculate the payload size manually
                int payload_len = PAYLOAD_SIZE;
                if (i == num_packets-1) {
                    payload_len = size - (PAYLOAD_SIZE * (num_packets-1));
                }

                Packet* new_packet = malloc(sizeof(Packet));
                new_packet->seq_num = -1;
                new_packet->ack_num = -1;
                new_packet->payload_len = payload_len;
                new_packet->offset = offset;
                new_packet->type = NONE;
                strcpy(new_packet->payload, payload);

                packet_list[i] = new_packet;
                offset += PAYLOAD_SIZE;
            }

            free(file_buffer);

            response = *packet_list[curr_packet];
            response.seq_num = received.ack_num;
            response.ack_num = received.seq_num + received.payload_len;
            curr_packet++;
        } else if (received.type == FIN_ACK) {
            // send ACK if FIN ACK is received
            response = packet_gen(received.ack_num, received.seq_num + received.payload_len+1, 0, size, ACK, NULL);
            fin_flag = 0;
        } else if (num_packets == curr_packet) {
            // send FIN if there are no more data packets to send
            response = packet_gen(received.ack_num, received.seq_num + received.payload_len, 0, size, FIN, NULL);
        } else {
            // send the file chunks
            response = *packet_list[curr_packet];
            response.seq_num = received.ack_num;
            response.ack_num = received.seq_num + received.payload_len;
            curr_packet++;
        }

        // send packet and print sending message
        send_packet(sockfd, (struct sockaddr*)&cli_addr, cli_addr_len, response);
        type = packet_type(response.type); 
        printf("Sending packet %d%s\n", response.seq_num, type); 
    }

    for (int i = 0; i < num_packets; i++) {
        free(packet_list[i]);
    }
    free(packet_list);
    return 0;
}