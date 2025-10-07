#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1" // Địa chỉ server LOOPBACK
#define SERVER_PORT 8085
#define BUFFER_SIZE 1024

int main()
{
    int sockfd;
    struct sockaddr_in servaddr;
    char buffer[BUFFER_SIZE];

    // Tạo socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Cấu hình địa chỉ server
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    for( ; ; ){
    // Gửi dữ liệu đến server
    const char *message = "Hello world";
    sendto(sockfd, message, strlen(message), 0,
           (const struct sockaddr *)&servaddr, sizeof(servaddr));
    printf("Sent to server: %s\n", message);

    // Nhận phản hồi từ server
    socklen_t len = sizeof(servaddr);
    int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                     (struct sockaddr *)&servaddr, &len);
    buffer[n] = '\0';

    printf("Received from server: %s\n", buffer);
    }
    close(sockfd);
    return 0;
}
