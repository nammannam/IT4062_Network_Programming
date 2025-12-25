#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include "llist.h"
#include "protocol.h"

#define MAX_LEN 20
#define BUFFER_SIZE 1024

int isLoggedIn = 0; // 0 - no user logged in, 1 - user logged in

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
    char buffer[BUFFER_SIZE];
    char username[MAX_LEN];
    char password[MAX_LEN];
    memset(username, 0, sizeof(username));
    memset(password, 0, sizeof(password));

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

    char cmd[MAX_LEN];
    char line[BUFFER_SIZE];  // Để đọc toàn bộ dòng

    application_msg_t msgIn;
    application_msg_t msgOut;
    memset(&msgOut, 0, sizeof(msgOut));
    memset(&msgIn, 0, sizeof(msgIn));

    for (;;)
    {

        if (fgets(line, sizeof(line), stdin) == NULL)
        {
            break; // EOF hoặc lỗi
        }

        // Loại bỏ newline từ fgets
        line[strcspn(line, "\n")] = '\0';

        // Tạo và gửi message LOGIN đến server
        if(isLoggedIn == 0){

            // Tạo và gửi message đến server (LOGIN hoặc PASSWORD)
            create_message(&msgOut, MSG_LOGIN, line);
            send(sockfd, &msgOut, sizeof(msgOut), 0);

            // Nhận phản hồi từ server
            int n = recv(sockfd, &msgIn, sizeof(msgIn), 0);

        
            if (n <= 0) {
                printf("Server closed connection or error.\n");
                break;
            } else {
                // Xử lý phản hồi từ server
                if(msgIn.type == MSG_PASSWORD){
                    // Server yêu cầu nhập password
                    client_handle_response(&msgIn);
                    
                    // Nhập password
                    if (fgets(line, sizeof(line), stdin) == NULL) {
                        break;
                    }
                    line[strcspn(line, "\n")] = '\0';
                    
                    // Gửi password
                    create_message(&msgOut, MSG_PASSWORD, line);
                    send(sockfd, &msgOut, sizeof(msgOut), 0);
                    
                    // Nhận phản hồi sau khi gửi password
                    n = recv(sockfd, &msgIn, sizeof(msgIn), 0);
                    if (n <= 0) {
                        printf("Server closed connection or error.\n");
                        break;
                    }
                    
                    client_handle_response(&msgIn);
                    if(msgIn.type == MSG_CF){
                        isLoggedIn = 1; // Đăng nhập thành công
                    }
                } else {
                    // Xử lý phản hồi khác (DENY, CF...)
                    client_handle_response(&msgIn);
                    if(msgIn.type == MSG_CF){
                        isLoggedIn = 1; // Đăng nhập thành công (trường hợp cũ)
                    }
                }
            }
        }else if(isLoggedIn == 1){

            if(strcmp(line, "bye") == 0){

                // Tạo và gửi message LOGOUT đến server
                create_message(&msgOut, MSG_TEXT, line);
                send(sockfd, &msgOut, sizeof(msgOut), 0);

                // Nhận phản hồi từ server
                int n = recv(sockfd, &msgIn, sizeof(msgIn), 0);
                if (n <= 0) {
                    printf("Server closed connection or error.\n");
                    break;
                }else{
                    // Xử lý phản hồi từ server
                    client_handle_response(&msgIn);
                    isLoggedIn = 0; // Cập nhật trạng thái đăng xuất
                }
                continue;
            }

            if(strcmp(line, "who") == 0){
                // Tạo và gửi message WHO đến server
                create_message(&msgOut, MSG_WHO, "");
                send(sockfd, &msgOut, sizeof(msgOut), 0);

                // Nhận phản hồi từ server
                int n = recv(sockfd, &msgIn, sizeof(msgIn), 0);
                if (n <= 0) {
                    printf("Server closed connection or error.\n");
                    break;
                }else{
                    // Xử lý phản hồi từ server (MSG_LIST)
                    client_handle_response(&msgIn);
                }
                continue;
            }

            if(strcmp(line, "help") == 0){
                // Tạo và gửi message HELP đến server
                create_message(&msgOut, MSG_HELP, "");
                send(sockfd, &msgOut, sizeof(msgOut), 0);

                // Nhận phản hồi từ server
                int n = recv(sockfd, &msgIn, sizeof(msgIn), 0);
                if (n <= 0) {
                    printf("Server closed connection or error.\n");
                    break;
                }else{
                    // Xử lý phản hồi từ server (MSG_CF với help text)
                    client_handle_response(&msgIn);
                }
                continue;
            }

            // Gửi TEXT message đến server
            create_message(&msgOut, MSG_TEXT, line);
            send(sockfd, &msgOut, sizeof(msgOut), 0);

            // Nhận phản hồi từ server
            int n = recv(sockfd, &msgIn, sizeof(msgIn), 0);
            if (n <= 0) {
                printf("Server closed connection or error.\n");
                break;
            }else{
                // Xử lý phản hồi từ server
                client_handle_response(&msgIn);
            }
        }
        
        /*
        // Nếu dòng rỗng (người dùng chỉ nhấn ENTER), kết thúc client
        if (strlen(line) == 0) {
            // Nếu đang đăng nhập, gửi "bye" trước khi thoát
            if (isLoggedIn == 1) {
                strcpy(buffer, "bye");
                send(sockfd, buffer, strlen(buffer), 0);
                
                // Nhận phản hồi từ server
                int n = recv(sockfd, buffer, BUFFER_SIZE, 0);

                



                if (n > 0) {
                    buffer[n] = '\0';
                    printf("Server: %s", buffer);
                }
            }
            printf("Client terminated.\n");
            break;
        }

        // Copy vào cmd (hoặc xử lý trực tiếp)
        strcpy(cmd, line);
        
        if (strcmp(cmd, "exit") == 0)
        {
            break;
        }
        
        if (strcmp(cmd, "bye") == 0 && isLoggedIn == 1)
        {
            // Gửi "bye" đến server
            strcpy(buffer, "bye");
            send(sockfd, buffer, strlen(buffer), 0);
            
            // Nhận phản hồi từ server
            int n = recv(sockfd, buffer, BUFFER_SIZE, 0);
            if (n > 0) {
                buffer[n] = '\0';
                printf("Server: %s", buffer);
            }
            
            memset(username, 0, sizeof(username)); // Reset username để đăng nhập lại
            memset(password, 0, sizeof(password)); // Reset password để đăng nhập lại
            isLoggedIn = 0;                        // Reset trạng thái đăng nhập để đăng nhập lại
            continue; // Tiếp tục vòng lặp, không thoát client
        }

        // Gửi username hoặc password đến server
        // Kiểm tra để tránh gửi chuỗi rỗng
        if (strlen(cmd) == 0) {
            continue;
        }

        strcpy(buffer, cmd);
        // sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        send(sockfd, buffer, strlen(buffer), 0);

        // Nhận phản hồi từ server
        socklen_t servaddr_len = sizeof(servaddr);

        //Note: Nhớ recvfrom là blocking function nên server phải trả lại gì đó mới tiếp tục được 
        // int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&servaddr, &servaddr_len);
        int n = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (n <= 0) {
            printf("Server closed connection or error.\n");
            break;
        }
        buffer[n] = '\0';

        if (strcmp(buffer, "OK\n") == 0)
        {
            isLoggedIn = 1;
            printf("Login successful.\n");
        }

        printf("Server: %s", buffer);

        */
    }

    




    close(sockfd);

    return 0;
}