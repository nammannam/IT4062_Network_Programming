CC = gcc
# CFLAGS = -Wall -Wextra -std=c99
TARGET1 = client_b2
TARGET2 = server_b2
SOURCE1 = client_b2.c
SOURCE2 = server_b2.c

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(SOURCE1)
	$(CC) -o $(TARGET1) $(SOURCE1)

$(TARGET2): $(SOURCE2)
	$(CC) -o $(TARGET2) $(SOURCE2)

clean:
	rm -f $(TARGET1) $(TARGET2)

.PHONY: all clean