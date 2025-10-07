#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8085
#define BUFFER_SIZE 1024

int main()
{
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len = sizeof(cliaddr);

    // Tạo socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Cấu hình địa chỉ server
    servaddr.sin_family = AF_INET;         // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY; // Lắng nghe tất cả địa chỉ IP
    servaddr.sin_port = htons(PORT);       // Cổng

    // Gắn địa chỉ IP và port cho socket
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    for( ; ; ){
    printf("Listening on port %d...\n", PORT);

    // Nhận dữ liệu từ client
    int n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, 0,
                     (struct sockaddr *)&cliaddr, &len);

    buffer[n] = '\0';
    printf("Server received from client (%s:%d) : %s\n",
           inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), buffer);

    // Lấy IP và port của server
    char server_ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &servaddr.sin_addr, server_ip, INET_ADDRSTRLEN);

    // Tạo chuỗi phản hồi
    char reply[BUFFER_SIZE];

    snprintf(reply, sizeof(reply),
             "Hello (%s:%d) from (%s:%d)",
             inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port),
             server_ip, PORT);

    // Gửi phản hồi lại client
    sendto(sockfd, reply, strlen(reply), 0,
           (const struct sockaddr *)&cliaddr, len);
           
    printf("Reply sent to client DONE.\n");
}
    close(sockfd);
    return 0;
}
