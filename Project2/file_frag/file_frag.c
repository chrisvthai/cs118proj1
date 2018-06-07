#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define PACKET_SIZE 1024
#define PAYLOAD_SIZE (PACKET_SIZE - sizeof(int) - sizeof(enum Type))
#define WINDOW_SIZE 5120

enum Type {NONE, SYN, SYN_ACK, ACK, FIN, FIN_ACK};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ./%s filename\n", argv[0]);
        exit(1);
    }

    //Open filestream
    char* file = argv[1];
    FILE* open_this = fopen(file, "r");
    if (open_this == NULL) {
        perror("Error, unable to open file to read from.");
        exit(1);
    }

    //Get file length and read file into buffer
    fseek(open_this, 0, SEEK_END);
    long int size = ftell(open_this);
    fseek(open_this, 0, SEEK_SET);

    char *buffer = (char *) malloc(size);
    if (fread(buffer, size, 1, open_this) < 0) {
        perror("Error reading from file");
        exit(1);
    }
    fclose(open_this);

    //Determine how many packets are needed to transmit the entire file
    int num_packets = (int) (size/PAYLOAD_SIZE + 1);

    //Incrementally copy some of the buffer into a packet, then write packet to 
    //some output file
    FILE* write_to = fopen("output.data", "a");
    char payload[PAYLOAD_SIZE];
    int offset = 0;
    
    /*for (int i = 0; i < num_packets; i++) {
        memcpy(payload, buffer+offset, PAYLOAD_SIZE);
        offset += PAYLOAD_SIZE;
        fwrite(payload, PAYLOAD_SIZE, 1, write_to);
    }*/
    int i = 0;
    int* packets_to_send = malloc(num_packets*sizeof(int)); //A list of seq numbers of the packet
    int start_seq = 0; //TODO: Start seq at 0 for now, figure out actual start later (i.e. random number?)

    //Figure out what the seq nums for each of the packets we're going to send are
    for (int j = 0; j < num_packets; j++) {
        int seq_num = start_seq + (j*PACKET_SIZE);
        while (seq_num > 30720)
            seq_num -= 30720;
        
        packets_to_send[j] = seq_num; 
    }

    //DEBUG: print out the expected seq nums
    for (int i = 0; i < num_packets; i++)
        printf("%d, ", packets_to_send[i]);

    int start_wnd = 0;
    int wnd_size = 4; //5 - 1
    int last_ack = -1;

    while (i < num_packets && last_ack < num_packets) { //i represents the latest packet we sent, TODO: we'll have to figure out how to retransmit a packet
        //Start sending all packets starting from the 1st one, but only up to start_wnd + wnd_size
        //We only move start_wnd up one once we get confirmation it went through properly
        memcpy(payload, buffer+(i*PAYLOAD_SIZE), PAYLOAD_SIZE);

        if (i >= start_wnd && i <= start_wnd + wnd_size) { //the packet is within the window, send it
            fwrite(payload, PAYLOAD_SIZE, 1, write_to);
            
            last_ack = i; //TODO: for now, the file writing is the confirmation of "receipt"
      
            printf("Sending packet with seq number: %d\n", packets_to_send[i]);
            printf("Received ACK number :%d\n", packets_to_send[i]);
            i++;
        }

        if (last_ack == start_wnd) { //The last ack num corresponds to the first packet in the window (TODO: can't handle packet loss yet
            start_wnd += 1;
            printf("Moved window up by one packet. start_wnd is now %d\n", packets_to_send[start_wnd]);
        }
    }

    fclose(write_to);
    free(buffer);
}

