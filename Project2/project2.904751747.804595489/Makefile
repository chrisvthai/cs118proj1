# TODO: Test that this compiles everything properly

CC = gcc
CFLAGS = -lm
SERVER_FILES = helper.c server.c 
CLIENT_FILES = helper.c client.c 
ALL_FILES = client server received.data

default: clean server client

server: $(SERVER_FILES)
	$(CC) $^ -o $@ $(CFLAGS)

client: $(CLIENT_FILES)
	$(CC) $^ -o $@ $(CFLAGS)

clean:
	-rm $(ALL_FILES)
