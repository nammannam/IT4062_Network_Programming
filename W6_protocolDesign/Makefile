CC = gcc
# CFLAGS = -Wall -Wextra -std=c99
TARGET1 = client
TARGET2 = server
SOURCE1 = client.c
SOURCE2 = server.c

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(SOURCE1)
	$(CC) -o $(TARGET1) $(SOURCE1)

$(TARGET2): $(SOURCE2)
	$(CC) -o $(TARGET2) $(SOURCE2)

clean:
	rm -f $(TARGET1) $(TARGET2)

.PHONY: all clean