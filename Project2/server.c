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
        perror("ERROR opening socket");
    memset((char *) &serv_addr, 0, sizeof(serv_addr));  // reset memory

    // fill in address info
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        perror("ERROR on binding");

    socklen_t cli_addr_len = sizeof(cli_addr);

    listen(sockfd, 5);  // 5 simultaneous connection at most
    
    // accept connections 
    // newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    // if (newsockfd < 0)
    //   perror("ERROR on accept");

    // int n;
    // char buffer[256];

    //  memset(buffer, 0, 256);  // reset memory

    //  //read client's message
    // n = read(newsockfd, buffer, 255);
    // if (n < 0) perror("ERROR reading from socket");
    // printf("Here is the message: %s\n", buffer);

    // //reply to client
    // n = write(newsockfd, "I got your message", 18);
    // if (n < 0) perror("ERROR writing to socket");

    // close(newsockfd);  // close connection
    // close(sockfd);

    // assumption that the requested file is in the current directory

    // set the timeout for receiving packets from the socket
    // setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&TIMEOUT, sizeof(TIMEOUT));

    // array of packets for easy access
    Packet** packet_list = 0;
    int num_packets = 0;
    //int curr_packet = 0;
    int fin_flag = 1;
    long int size = -1; // size of file

    //Window and timeout management
    int sent_packets = 0;
    int start_wnd = 0;
    int wnd_size = 4; //For sweeping through the window
    int resend_flag = 0;
    int sending_data = 0; //1 if it's transferring the file rn
    fd_set rfds;
    struct timeval timeout;
    int timed_out;

    timeout.tv_sec = 0;
    timeout.tv_sec = 500000;

    while(fin_flag) {

        // retrieve the packet
	
        //TODO: Should there be a select call here to implement the timeout??
        //Should we then have another conditional branch that'll handle it?
        Bytes to_get;
	Packet response;

        //Timeout check
        FD_ZERO(&rfds);
        FD_SET(sockfd, &rfds);

        timed_out = select(sockfd+1, &rfds, NULL, NULL, &timeout);
        if (timed_out < 1 && sending_data) {
            //Insert timeout code
            for (int i = start_wnd; i < start_wnd + wnd_size && i < num_packets; i++) {
               
                response = *packet_list[i];
                packet_list[i]->sent = 1;
                //response.seq_num = received.ack_num;
                //response.ack_num = received.seq_num + received.payload_len;
                
                send_packet(sockfd, (struct sockaddr*)&cli_addr, cli_addr_len, response);
                char* type = packet_type(response.type);
                
                printf("Sending packet %d Retransmission%s\n", response.seq_num, type);     
            }
            continue;
        }

        //If we didn't timeout, receive the ACK as normal
        ssize_t n = recvfrom(sockfd, to_get.bytes, PACKET_SIZE, 0, (struct sockaddr*) &cli_addr, &cli_addr_len);

        Packet received = to_get.my_packet;

        // print receiving message
        char* type = packet_type(received.type); 
        printf("Receiving packet %d%s\n", received.ack_num, type);


        // create the SYN ACK
        if (received.type == SYN) {
            response = packet_gen(0, 1, 0, 0, SYN_ACK, NULL);

            // send packet and print sending message
            send_packet(sockfd, (struct sockaddr*)&cli_addr, cli_addr_len, response);
            type = packet_type(response.type); 
            printf("Sending packet %d%s\n", response.seq_num, type); 
            continue;
        } else if (received.type == ACK && size == -1) {
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
                new_packet->received = 0;
                new_packet->sent = 0;
                new_packet->type = NONE;
                strcpy(new_packet->payload, payload);

                packet_list[i] = new_packet;
                offset += PAYLOAD_SIZE;
            }

            free(file_buffer);
            sending_data = 1;
            printf("Finished dividing up file into %d packets\n", num_packets);

            // response = *packet_list[curr_packet];
            // response.seq_num = received.ack_num;
            // response.ack_num = received.seq_num + received.payload_len;
            // curr_packet++;
        } else if (received.type == FIN_ACK) {
            // send ACK if FIN ACK is received
            response = packet_gen(received.ack_num, received.seq_num + received.payload_len+1, 0, size, ACK, NULL);
            fin_flag = 0;

            // send packet and print sending message
            send_packet(sockfd, (struct sockaddr*)&cli_addr, cli_addr_len, response);
            type = packet_type(response.type); 
            printf("Sending packet %d%s\n", response.seq_num, type); 
            continue;
        } else if (sent_packets == num_packets) {
            // send FIN if there are no more data packets to send
            //printf("sent_packets:%d is equal to num_packets:%d");
            response = packet_gen(received.ack_num, received.seq_num + received.payload_len, 0, size, FIN, NULL);

            // send packet and print sending message
            send_packet(sockfd, (struct sockaddr*)&cli_addr, cli_addr_len, response);
            type = packet_type(response.type); 
            printf("Sending packet %d%s\n", response.seq_num, type); 
            sending_data = 0;
            continue;
        }

        // mark packets in list as received if they were received
        // if (packet_list != NULL) {
        //     for (int i = 0; i < num_packets; i++) {
        //         if (packet_list[i]->offset == received.ack_num) {
        //             packet_list[i]->received = 1;
        //         }
        //     }
        // }

        /**************************************************************************************************/
        //If we reach this block of code, we send packets as normal

        if (sending_data) {
            //If timeout, should've gone to another branch
            //Otherwise, receive the ACK
            for (int i = 0; i < num_packets; i++) {
                if ((i+1)*PACKET_SIZE == received.ack_num-1 && i >= start_wnd - 1) {
                    packet_list[i]->received += 1;
		    //printf("Client has received packet %d\n", received.ack_num);
                    if (i == start_wnd) {
                        start_wnd++;
                        sent_packets++;
                        //printf("Number of sent packets: %d\n", sent_packets);
			//printf("Start_wnd: %d\n", start_wnd);
                    }
                }
            }
	   // printf("checking for dup acks\n");
            for (int a = 0; a < num_packets; a++) { //On a dup ack, resend all packets in window
		//printf("checking packet %d\n", a);
		//printf("packet_list[%d]->received == %d\n", a, packet_list[a]->received);
                if (packet_list[a]->received == 4) {
		    printf("EGAD dup ack!\n");
                    packet_list[a]->received = 0;
                    for (int k = start_wnd; k <= start_wnd + wnd_size && k < num_packets; k++) {
                        packet_list[k]->sent = 0;
                        resend_flag = 1;
                    }
		    break;
                }
            }

	    //printf("sending unsent packets in window\n");
            //After receiving ACK, send stuff in the window that isn't already sent
            for (int b = start_wnd; b <= start_wnd + wnd_size && b < num_packets; b++) {
		//printf("%d\n", b);
                if (packet_list[b]->sent == 0) {
                    response = *packet_list[b];
                    packet_list[b]->sent = 1;
                    response.seq_num = received.ack_num+(sizeof(response)*(b-start_wnd));
                    response.ack_num = received.seq_num + sizeof(received);
                
                    send_packet(sockfd, (struct sockaddr*)&cli_addr, cli_addr_len, response);
                    type = packet_type(response.type);
                    if (resend_flag) {
                        printf("Sending packet %d Retransmission%s\n", response.seq_num, type);
                        resend_flag = 0;
                    }
                    else { 
                        printf("Sending packet %d%s\n", response.seq_num, type);
                        //printf("It's payload is %s\n", response.payload);
                    }

                }
            }
	    if (sent_packets == num_packets) {
                // send FIN if there are no more data packets to send
                response = packet_gen(received.ack_num+PACKET_SIZE, received.seq_num + sizeof(received), 0, size, FIN, NULL);

                // send packet and print sending message
                send_packet(sockfd, (struct sockaddr*)&cli_addr, cli_addr_len, response);
                type = packet_type(response.type); 
                printf("Sending packet %d%s\n", response.seq_num, type); 

                sending_data = 0;
            }  
        }
        /*
        response = *packet_list[curr_packet];
        packet_list[curr_packet]->sent = 1;
        response.seq_num = received.ack_num;
        response.ack_num = received.seq_num + received.payload_len;
        curr_packet++;
        */

        // send packet and print sending message
       // send_packet(sockfd, (struct sockaddr*)&cli_addr, cli_addr_len, response);
        //type = packet_type(response.type); 
       // printf("Sending packet %d%s\n", response.seq_num, type); 
        /***************************************************************************************************/
    }

    free(packet_list);
    return 0;
}
