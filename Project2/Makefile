# TODO: Test that this compiles everything properly

CC = gcc
CLFLAGS = -O2 -std=c99
SERVER_FILES = server.c header.c
CLIENT_FILES = client.c header.c

default: server client

server: $(SERVER_FILES)
	@$(CC) $(CFLAGS) -o $@ $^

client: $(CLIENT_FILES)
	@$(CC) $(CFLAGS) -o $@ $^