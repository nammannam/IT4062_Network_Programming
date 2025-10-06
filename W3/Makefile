CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET1 = resolver_b1
TARGET2 = resolver_b2
SOURCE1 = week3_b1.c
SOURCE2 = week3_b2.c

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(SOURCE1)
	$(CC) $(CFLAGS) -o $(TARGET1) $(SOURCE1)

$(TARGET2): $(SOURCE2)
	$(CC) $(CFLAGS) -o $(TARGET2) $(SOURCE2)

clean:
	rm -f $(TARGET1) $(TARGET2) resolver.log

.PHONY: all clean