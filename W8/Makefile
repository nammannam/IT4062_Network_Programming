# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -pthread

# Header files and common objects
DEPS = protocol.h llist.h
COMMON_OBJS = protocol_utils.o llist_utils.o

# Target executables
TARGETS = client server

# Default target
all: $(TARGETS)

# Pattern rule for object files
%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

# Client executable
client: client.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Server executable
server: server.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Clean up
clean:
	rm -f *.o $(TARGETS)

# Phony targets
.PHONY: all clean