CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET1 = client_b2
TARGET2 = server_b2_select
TARGET3 = server_b2_poll
SOURCE1 = client_b2.c
SOURCE2 = server_b2_select.c
SOURCE3 = server_b2_poll.c

all: $(TARGET1) $(TARGET2) $(TARGET3)

$(TARGET1): $(SOURCE1)
	$(CC) $(CFLAGS) -o $(TARGET1) $(SOURCE1)

$(TARGET2): $(SOURCE2)
	$(CC) $(CFLAGS) -o $(TARGET2) $(SOURCE2)

$(TARGET3): $(SOURCE3)
	$(CC) $(CFLAGS) -o $(TARGET3) $(SOURCE3)

clean:
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) 

.PHONY: all clean