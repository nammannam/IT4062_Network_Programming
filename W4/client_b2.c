#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "llist.h"

#define MAX_LEN 20
#define BUFFER_SIZE 1024

int isLoggedIn = 0; // 0 - no user logged in, 1 - user logged in

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

    // Tạo socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Cấu hình địa chỉ server
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(servPort);
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    char cmd[MAX_LEN];
    char line[BUFFER_SIZE];  // Để đọc toàn bộ dòng

    for (;;)
    {

        if (fgets(line, sizeof(line), stdin) == NULL)
        {
            break; // EOF hoặc lỗi
        }

        // Loại bỏ newline từ fgets
        line[strcspn(line, "\n")] = '\0';

        // Nếu dòng rỗng, bỏ qua
        if (strlen(line) == 0)
            continue;

        // Copy vào cmd (hoặc xử lý trực tiếp)
        strcpy(cmd, line);
        if (strcmp(cmd, "exit") == 0)
        {
            break;
        }
        if (strcmp(cmd, "bye") == 0 && isLoggedIn == 1)
        {
            // Gửi "bye" đến server
            //  strcpy(buffer, cmd);
            //  sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
            printf("Disconnected from server.\n");
            memset(username, 0, sizeof(username)); // Reset username để đăng nhập lại
            memset(password, 0, sizeof(password)); // Reset password để đăng nhập lại
            isLoggedIn = 0;                        // Reset trạng thái đăng nhập để đăng nhập lại
        }

        // Gửi username hoặc password đến server

        strcpy(buffer, cmd);
        sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));

        // Nhận phản hồi từ server
        socklen_t servaddr_len = sizeof(servaddr);

        //Note: Nhớ recvfrom là blocking function nên server phải trả lại gì đó mới tiếp tục được 
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&servaddr, &servaddr_len);
        buffer[n] = '\0';

        if (strcmp(buffer, "OK\n") == 0)
        {
            isLoggedIn = 1;
            printf("Login successful.\n");
        }

        printf("Server: %s", buffer);
    }

    close(sockfd);
    return 0;
}