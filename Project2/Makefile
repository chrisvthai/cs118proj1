# TODO: Test that this compiles everything properly

CC = gcc
CLFLAGS = -O2
SERVER_FILES = helper.c server.c 
CLIENT_FILES = helper.c client.c 
ALL_FILES = client server received.data

default: clean server client

server: $(SERVER_FILES)
	$(CC) $^ $(CFLAGS) -o $@ 

client: $(CLIENT_FILES)
	$(CC) $^ $(CFLAGS) -o $@

clean:
	-rm $(ALL_FILES)