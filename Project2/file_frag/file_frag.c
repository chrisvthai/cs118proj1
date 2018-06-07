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
    
    for (int i = 0; i < num_packets; i++) {
        memcpy(payload, buffer+offset, PAYLOAD_SIZE);
        offset += PAYLOAD_SIZE;
        fwrite(payload, PAYLOAD_SIZE, 1, write_to);
    }

    fclose(write_to);
    free(buffer);
}

