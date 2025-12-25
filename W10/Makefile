# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

DEPS = protocol.h llist.h
COMMON_OBJS = protocol_utils.o llist_utils.o

TARGETS = client server client_nonblocking

all: $(TARGETS)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

client: client.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

client_nonblocking: client_nonblocking.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

server: server.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean