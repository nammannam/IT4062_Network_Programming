#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include "llist.h"
#include "protocol.h"

#define MAX_LEN 20
#define BUFFER_SIZE 1024

int isLoggedIn = 0; // 0 - no user logged in, 1 - user logged in
int waitingForPassword = 0; // 0 - waiting for username, 1 - waiting for password

// Set file descriptor to non-blocking mode
int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        return -1;
    }
    return 0;
}

// Set terminal to raw mode for character-by-character input
struct termios orig_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Lấy server port
    char *endptr;
    long servPort_l = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0' || servPort_l <= 0 || servPort_l > 65535)
    {
        fprintf(stderr, "Invalid port: %s\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    uint16_t servPort = (uint16_t)servPort_l;

    int sockfd;
    struct sockaddr_in servaddr;
    char line[BUFFER_SIZE];

    // Tạo socket TCP
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Cấu hình địa chỉ server
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(servPort);

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }else{
        printf("Connected to server (%s:%d)\n", argv[1], servPort);
    }

    // Set socket to non-blocking mode
    if (set_nonblocking(sockfd) < 0) {
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Enable raw mode for terminal input
    enable_raw_mode();

    printf("Client running in non-blocking mode with select()\n");
    printf("Enter username to login:\n");

    application_msg_t msgIn;
    application_msg_t msgOut;
    memset(&msgOut, 0, sizeof(msgOut));
    memset(&msgIn, 0, sizeof(msgIn));

    char input_buffer[BUFFER_SIZE];
    int input_pos = 0;
    memset(input_buffer, 0, sizeof(input_buffer));

    fd_set read_fds;
    int running = 1;

    while (running)
    {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sockfd, &read_fds);
        
        int max_fd = (sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;
        
        // Wait for activity on stdin or socket
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        
        if (activity < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }

        // Check for data from server
        if (FD_ISSET(sockfd, &read_fds)) {
            ssize_t n = recv(sockfd, &msgIn, sizeof(msgIn), 0);
            
            if (n <= 0) {
                if (n == 0) {
                    printf("\nServer closed connection.\n");
                } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("recv");
                }
                running = 0;
                break;
            } else if (n == sizeof(msgIn)) {
                // Complete message received
                printf("\n");  // New line before server response
                
                // Check if server is asking for password
                if (msgIn.type == MSG_PASSWORD) {
                    waitingForPassword = 1;
                }
                
                client_handle_response(&msgIn);
                
                // Update login status
                if(msgIn.type == MSG_CF && isLoggedIn == 0){
                    isLoggedIn = 1;
                    waitingForPassword = 0;
                } else if (msgIn.type == MSG_CF && strcmp(msgIn.value, "Goodbye") == 0) {
                    isLoggedIn = 0;
                    waitingForPassword = 0;
                    running = 0;
                    break;
                } else if (msgIn.type == MSG_DENY) {
                    // Reset password waiting state on denial
                    waitingForPassword = 0;
                }
                
                // Re-display input prompt
                if (running) {
                    printf("%.*s", input_pos, input_buffer);
                    fflush(stdout);
                }
            }
        }

        // Check for user input from stdin
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char ch;
            ssize_t n = read(STDIN_FILENO, &ch, 1);
            
            if (n > 0) {
                if (ch == '\n' || ch == '\r') {
                    // Complete line received
                    printf("\n");
                    input_buffer[input_pos] = '\0';
                    
                    if (input_pos > 0) {
                        // Process the input
                        if(isLoggedIn == 0){
                            // Check if we're waiting for password
                            if (waitingForPassword) {
                                // Send password message
                                create_message(&msgOut, MSG_PASSWORD, input_buffer);
                                send(sockfd, &msgOut, sizeof(msgOut), 0);
                            } else {
                                // Send login message (username)
                                create_message(&msgOut, MSG_LOGIN, input_buffer);
                                send(sockfd, &msgOut, sizeof(msgOut), 0);
                            }
                        } else {
                            // Logged in - process commands
                            if(strcmp(input_buffer, "bye") == 0){
                                create_message(&msgOut, MSG_TEXT, input_buffer);
                                send(sockfd, &msgOut, sizeof(msgOut), 0);
                            } else if(strcmp(input_buffer, "who") == 0){
                                create_message(&msgOut, MSG_WHO, "");
                                send(sockfd, &msgOut, sizeof(msgOut), 0);
                            } else if(strcmp(input_buffer, "help") == 0){
                                create_message(&msgOut, MSG_HELP, "");
                                send(sockfd, &msgOut, sizeof(msgOut), 0);
                            } else {
                                // Regular text message
                                create_message(&msgOut, MSG_TEXT, input_buffer);
                                send(sockfd, &msgOut, sizeof(msgOut), 0);
                            }
                        }
                    }
                    
                    // Reset input buffer
                    input_pos = 0;
                    memset(input_buffer, 0, sizeof(input_buffer));
                } else if (ch == 127 || ch == 8) {
                    // Backspace
                    if (input_pos > 0) {
                        input_pos--;
                        printf("\b \b");
                        fflush(stdout);
                    }
                } else if (ch >= 32 && ch < 127 && input_pos < BUFFER_SIZE - 1) {
                    // Printable character
                    input_buffer[input_pos++] = ch;
                    // Echo character
                    write(STDOUT_FILENO, &ch, 1);
                }
            } else if (n == 0) {
                // EOF on stdin
                printf("\nEOF detected. Closing connection.\n");
                if (isLoggedIn) {
                    create_message(&msgOut, MSG_TEXT, "bye");
                    send(sockfd, &msgOut, sizeof(msgOut), 0);
                }
                running = 0;
                break;
            }
        }
    }

    close(sockfd);
    
    // Terminal cleanup is handled by atexit
    printf("\nClient terminated.\n");

    return 0;
}
