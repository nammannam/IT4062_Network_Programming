#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<poll.h>
#include<fcntl.h>
#include<errno.h>
#include "llist.h"

#define INPUT_FILE_PATH "./account.txt"
#define HISTORY_FILE_PATH "./history.txt"
#define MAX_LEN 20
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

// Cấu trúc lưu trạng thái của mỗi client
typedef struct {
    int sockfd;                     // Socket file descriptor
    struct sockaddr_in addr;        // Địa chỉ client
    int is_logged_in;               // Trạng thái đăng nhập: 0=chưa, 1=đã đăng nhập
    char username[MAX_LEN];         // Username hiện tại
    char pending_username[MAX_LEN]; // Username đang chờ nhập password
    int attempts;                   // Số lần đăng nhập sai
    int active;                     // Slot đang sử dụng: 0=trống, 1=đang dùng
} ClientState_s;

ClientState_s clients[MAX_CLIENTS];  // Mảng lưu trạng thái các clients
int client_count = 0;                // Số lượng clients đang kết nối

// Khai báo hàm
void handle_client_message(int client_index, char *buffer, Llist_s *list, struct pollfd *poll_fds, int *nfds);

// Đọc danh sách tài khoản từ file
void readAccountsFromFile(char *filePath, Llist_s *list)
{
    FILE *fptr = fopen(filePath, "r");
    if (fptr == NULL)
    {
        printf("File %s could not be opened for reading!\n", filePath);
        exit(EXIT_FAILURE);
    }
    AccountInfo_s acc;
    while (fscanf(fptr, "%s %s %s %s %c\n", acc.username, acc.password, acc.email, acc.homepage, &acc.status) != EOF)
    {
        list = insertAtTail(list, acc);
    }

    fclose(fptr);
}

// Cập nhật danh sách tài khoản từ file
void updateAccountListFromFile(char *filePath, Llist_s *list)
{
    FILE *fptr = fopen(filePath, "r");
    if (fptr == NULL)
    {
        printf("File %s could not be opened for reading!\n", filePath);
        exit(EXIT_FAILURE);
    }
    AccountInfo_s acc;
    list->next = NULL; // Xóa danh sách hiện tại
    while (fscanf(fptr, "%s %s %s %s %c\n", acc.username, acc.password, acc.email, acc.homepage, &acc.status) != EOF)
    {
        list = insertAtTail(list, acc);
    }

    fclose(fptr);
}

// Ghi danh sách tài khoản ra file
void updateAccountToFile(char *filePath, Llist_s *list)
{
    FILE *fptr = fopen(filePath, "w");
    if (fptr == NULL)
    {
        printf("File %s could not be opened for writing!\n", filePath);
        return;
    }
    Llist_s *pt;
    for (pt = list->next; pt != NULL; pt = pt->next)
    {
        fprintf(fptr, "%s %s %s %s %c\n", pt->nodeInfo.username, pt->nodeInfo.password, pt->nodeInfo.email, pt->nodeInfo.homepage, pt->nodeInfo.status);
    }

    fclose(fptr);
}

// Thiết lập socket ở chế độ non-blocking
int set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        return -1;
    }
    return 0;
}

// Khởi tạo trạng thái client
void init_client_state(ClientState_s *client) {
    client->sockfd = -1;
    memset(&client->addr, 0, sizeof(client->addr));
    client->is_logged_in = 0;
    memset(client->username, 0, MAX_LEN);
    memset(client->pending_username, 0, MAX_LEN);
    client->attempts = 0;
    client->active = 0;
}

// Thêm client mới vào mảng
int add_client(int sockfd, struct sockaddr_in addr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].sockfd = sockfd;
            clients[i].addr = addr;
            clients[i].active = 1;
            client_count++;
            printf("Client %d added (%s:%d) - Total clients: %d\n", 
                   i, 
                   inet_ntoa(addr.sin_addr),
                   ntohs(addr.sin_port),
                   client_count);
            return i;
        }
    }
    printf("Error: Maximum clients reached\n");
    return -1;
}

// Tìm index của client trong mảng clients dựa vào sockfd
int find_client_by_sockfd(int sockfd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].sockfd == sockfd) {
            return i;
        }
    }
    return -1;
}

// Xóa client khỏi mảng
void remove_client(int index, Llist_s *list) {
    (void)list; // Không sử dụng nhưng giữ để tương thích
    
    if (index < 0 || index >= MAX_CLIENTS || !clients[index].active) {
        return;
    }
    
    printf("Client %d disconnected (%s:%d)\n", 
           index,
           inet_ntoa(clients[index].addr.sin_addr),
           ntohs(clients[index].addr.sin_port));
    
    close(clients[index].sockfd);
    init_client_state(&clients[index]);
    client_count--;
}

// Cập nhật trạng thái tài khoản trong file
void updateAccountStatusInFile(char *filePath, char *username, char newStatus)
{
    FILE *fptr = fopen(filePath, "r+");
    if (fptr == NULL)
    {
        printf("File %s could not be opened for updating!\n", filePath);
        return;
    }

    char fileUsername[MAX_LEN];
    char filePassword[MAX_LEN];
    char fileEmail[MAX_LEN];
    char fileHomepage[MAX_LEN];
    char fileStatus;

    long pos;
    while ((pos = ftell(fptr)) != -1 && fscanf(fptr, "%s %s %s %s %c\n", fileUsername, filePassword, fileEmail, fileHomepage, &fileStatus) != EOF)
    {
        if (strcmp(fileUsername, username) == 0)
        {
            fseek(fptr, pos + strlen(fileUsername) + strlen(filePassword) + strlen(fileEmail) + strlen(fileHomepage) + 4, SEEK_SET);
            fputc(newStatus, fptr);
            break;
        }
    }

    fclose(fptr);
}

// Xử lý đăng nhập
int accountSignIn(Llist_s *list, char *filePath, char *username, char *password, int *attempts, ClientState_s *client)
{
    AccountInfo_s searchAcc = searchUsernameList(list, username);

    // Không tìm thấy Account
    if (strcmp(searchAcc.username, "") == 0)
    {
        printf("Cannot find account\n");
        return 0;
    }

    // Account bị khóa
    if (searchAcc.status == '0')
    {
        printf("Account is blocked\n");
        return 1;
    }

    if (strcmp(searchAcc.password, password) != 0)
    {
        printf("Incorrect password\n");
        (*attempts)++;

        // Khóa account sau 3 lần nhập sai password
        if (*attempts >= 3)
        {
            updateAccountStatusInFile(filePath, username, '0');
            updateAccountListFromFile(filePath, list);
            printf("Account blocked after 3 failed attempts\n");
            return 2;
        }

        printf("You have %d attempt(s) left\n", 3 - *attempts);
        return 3;
    }
    else
    {
        // Đăng nhập thành công
        *attempts = 0;
        strncpy(client->username, username, MAX_LEN - 1);
        client->username[MAX_LEN - 1] = '\0';
        client->is_logged_in = 1;
        return 4;
    }
}

// Xử lý đăng xuất
void accountSignOut(ClientState_s *client)
{
    if (!client->is_logged_in)
    {
        printf("No user is currently logged in.\n");
        return;
    }

    printf("User %s signed out successfully.\n", client->username);
    client->is_logged_in = 0;
    memset(client->username, 0, MAX_LEN);
}

// Xử lý chuỗi - tách số và chữ
char *stringProcess(char *str){
    char *temp1 = malloc(strlen(str) + 1);
    char *temp2 = malloc(strlen(str) + 1);
    char *res = malloc(strlen(str) + 1);
    memset(temp1, 0, strlen(str) + 1);
    memset(temp2, 0, strlen(str) + 1);
    memset(res, 0, strlen(str) + 1);

    for(size_t i = 0 ; i < strlen(str) ; i++){
        if(str[i] >= '0' && str[i] <= '9'){
            strncat(temp1, &str[i], 1);
        }
        if((str[i] >= 'a' && str[i] <= 'z') || (str[i] >= 'A' && str[i] <= 'Z')){
            strncat(temp2, &str[i], 1);
        }
    }
    
    if(strlen(temp1) == 0){
        sprintf(res, "\n%s\n", temp2);
        free(temp1);
        free(temp2);
    }else if(strlen(temp2) == 0){
        sprintf(res, "\n%s\n", temp1);
        free(temp1);
        free(temp2);
    }else{
        sprintf(res, "\n%s\n%s\n", temp1, temp2);
        free(temp1);
        free(temp2);
    }

    return res;
}

// Kiểm tra chuỗi chỉ chứa chữ và số
int stringCheck(char *str){
    for(size_t i = 0 ; i < strlen(str) ; i++){
        if(!(str[i] >= 'a' && str[i] <= 'z') && !(str[i] >= 'A' && str[i] <= 'Z') && !(str[i] >= '0' && str[i] <= '9')){
            return 0;
        }
    }
    return 1;
}

// Đổi mật khẩu
void changePassword(Llist_s *list, char *newPassword, char *reply, ClientState_s *client)
{
    // Tìm account trong danh sách
    Llist_s *pt = findNodeByUsername(list, client->username);
    if (pt == NULL)
    {
        printf("Error: Logged in user not found in the list.\n");
        sprintf(reply, "Error\n");
        return;
    }

    // Kiểm tra password hợp lệ
    if(stringCheck(newPassword) == 0){
        sprintf(reply, "Error\n");
        return;
    }

    // Cập nhật password
    strcpy(pt->nodeInfo.password, newPassword);
    updateAccountToFile(INPUT_FILE_PATH, list);

    char *replyResult = stringProcess(newPassword);
    sprintf(reply, "%s", replyResult);
    free(replyResult);
    printf("Password changed successfully.\n");
}

// Hàm xử lý message từ client
void handle_client_message(int client_index, char *buffer, Llist_s *list, struct pollfd *poll_fds, int *nfds) {
    ClientState_s *client = &clients[client_index];
    char reply[BUFFER_SIZE];
    memset(reply, 0, sizeof(reply));
    
    // Xử lý lệnh "bye"
    if (strcmp(buffer, "bye") == 0 && client->is_logged_in) {
        accountSignOut(client);
        memset(client->pending_username, 0, MAX_LEN);
        client->attempts = 0;
        sprintf(reply, "Goodbye\n");
        send(client->sockfd, reply, strlen(reply), 0);
        return;
    }
    
    // Client chưa đăng nhập
    if (!client->is_logged_in) {
        // Nếu chưa có pending_username -> đây là username
        if (strlen(client->pending_username) == 0) {
            strncpy(client->pending_username, buffer, MAX_LEN - 1);
            client->pending_username[MAX_LEN - 1] = '\0';
            
            // Kiểm tra username có tồn tại không
            AccountInfo_s searchAcc = searchUsernameList(list, client->pending_username);
            if (strcmp(searchAcc.username, "") == 0) {
                sprintf(reply, "Account not found\n");
                send(client->sockfd, reply, strlen(reply), 0);
                memset(client->pending_username, 0, MAX_LEN);
                return;
            }
            
            // Kiểm tra account có bị block không
            if (searchAcc.status == '0') {
                sprintf(reply, "Account is blocked\n");
                send(client->sockfd, reply, strlen(reply), 0);
                memset(client->pending_username, 0, MAX_LEN);
                return;
            }
            
            // Yêu cầu nhập password
            sprintf(reply, "Insert password\n");
            send(client->sockfd, reply, strlen(reply), 0);
        }
        // Đã có pending_username -> đây là password
        else {
            int signInResult = accountSignIn(list, INPUT_FILE_PATH, 
                                            client->pending_username, 
                                            buffer, 
                                            &client->attempts, 
                                            client);
            
            if (signInResult == 4) {
                // Đăng nhập thành công
                sprintf(reply, "OK\n");
                send(client->sockfd, reply, strlen(reply), 0);
            } else if (signInResult == 2) {
                // Account bị khóa sau 3 lần sai
                sprintf(reply, "Account is blocked\n");
                send(client->sockfd, reply, strlen(reply), 0);
                memset(client->pending_username, 0, MAX_LEN);
                client->attempts = 0;
            } else if (signInResult == 3) {
                // Sai password nhưng chưa đến 3 lần
                sprintf(reply, "Not OK\n");
                send(client->sockfd, reply, strlen(reply), 0);
            }
        }
    }
    // Client đã đăng nhập
    else {
        // Kiểm tra lệnh homepage
        if (strcmp(buffer, "homepage") == 0) {
            AccountInfo_s loggedInUser = searchUsernameList(list, client->username);
            sprintf(reply, "%s\n", loggedInUser.homepage);
            send(client->sockfd, reply, strlen(reply), 0);
        }
        // Lệnh đổi password
        else {
            changePassword(list, buffer, reply, client);
            send(client->sockfd, reply, strlen(reply), 0);
        }
    }
}

// ===== MAIN FUNCTION =====
int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: %s <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Lấy server port
    char *endptr;
    long servPort_l = strtol(argv[1], &endptr, 10);
    if(*endptr != '\0' || servPort_l <= 0 || servPort_l > 65535){
        fprintf(stderr, "Invalid port: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    uint16_t servPort = (uint16_t)servPort_l;
    
    // Đọc danh sách tài khoản từ file
    Llist_s *list = newList();
    readAccountsFromFile(INPUT_FILE_PATH, list);

    // Khởi tạo tất cả client slots
    for (int i = 0; i < MAX_CLIENTS; i++) {
        init_client_state(&clients[i]);
    }

    int listenfd;
    struct sockaddr_in servaddr;

    // Tạo socket TCP
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập socket non-blocking
    if (set_nonblocking(listenfd) < 0) {
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    // Cho phép reuse address
    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Cấu hình địa chỉ Server
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(servPort);

    // Gán IP và Port cho Socket
    if(bind(listenfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        perror("bind failed");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    // Chuyển sang trạng thái lắng nghe kết nối từ Client
    if(listen(listenfd, 10) < 0){
        perror("listen failed");
        close(listenfd);
        exit(EXIT_FAILURE);
    }
    
    printf("========================================\n");
    printf("Server is listening on port %d...\n", servPort);
    printf("Using poll() for I/O multiplexing\n");
    printf("Max clients: %d\n", MAX_CLIENTS);
    printf("========================================\n");

    // Khởi tạo mảng pollfd
    // poll_fds[0] sẽ là listenfd
    // poll_fds[1..MAX_CLIENTS] sẽ là các client sockets
    struct pollfd poll_fds[MAX_CLIENTS + 1];
    int nfds = 1; // Số lượng file descriptors đang theo dõi
    
    // Khởi tạo pollfd cho listenfd
    poll_fds[0].fd = listenfd;
    poll_fds[0].events = POLLIN; // Theo dõi sự kiện đọc (connection mới)
    
    // Khởi tạo các slots còn lại
    for (int i = 1; i <= MAX_CLIENTS; i++) {
        poll_fds[i].fd = -1; // -1 nghĩa là slot này chưa được sử dụng
        poll_fds[i].events = POLLIN;
    }

    // ===== VÒNG LẶP CHÍNH VỚI POLL() =====
    while(1) {
        // poll() sẽ block cho đến khi có sự kiện xảy ra trên bất kỳ fd nào
        // Tham số -1 nghĩa là timeout vô hạn
        int ret = poll(poll_fds, nfds, -1);
        
        if (ret < 0) {
            if (errno == EINTR) {
                continue; // Bị ngắt bởi signal, tiếp tục
            }
            perror("poll error");
            break;
        }

        // Kiểm tra listenfd có connection mới không
        if (poll_fds[0].revents & POLLIN) {
            struct sockaddr_in cliaddr;
            socklen_t cliaddr_len = sizeof(cliaddr);
            
            int connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
            if (connfd < 0) {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    perror("accept failed");
                }
            } else {
                // Thiết lập socket mới thành non-blocking
                if (set_nonblocking(connfd) < 0) {
                    close(connfd);
                } else {
                    // Thêm client vào mảng
                    int client_index = add_client(connfd, cliaddr);
                    if (client_index < 0) {
                        // Đã đạt max clients
                        char *msg = "Server is full. Try again later.\n";
                        send(connfd, msg, strlen(msg), 0);
                        close(connfd);
                    } else {
                        // Thêm vào mảng poll_fds
                        // Tìm slot trống trong poll_fds (fd == -1)
                        int added = 0;
                        for (int i = 1; i <= MAX_CLIENTS; i++) {
                            if (poll_fds[i].fd == -1) {
                                poll_fds[i].fd = connfd;
                                poll_fds[i].events = POLLIN;
                                if (i >= nfds) {
                                    nfds = i + 1;
                                }
                                added = 1;
                                break;
                            }
                        }
                        
                        if (!added) {
                            printf("Error: Could not add client to poll_fds\n");
                            remove_client(client_index, list);
                        }
                    }
                }
            }
        }

        // Kiểm tra các client sockets
        for (int i = 1; i < nfds; i++) {
            if (poll_fds[i].fd == -1) {
                continue; // Slot trống
            }
            
            // Kiểm tra sự kiện đọc
            if (poll_fds[i].revents & POLLIN) {
                int sockfd = poll_fds[i].fd;
                char buffer[BUFFER_SIZE];
                
                int rcvBytes = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
                
                if (rcvBytes <= 0) {
                    // Client ngắt kết nối hoặc lỗi
                    int client_index = find_client_by_sockfd(sockfd);
                    if (client_index >= 0) {
                        remove_client(client_index, list);
                    }
                    
                    // Xóa khỏi poll_fds
                    poll_fds[i].fd = -1;
                    
                    // Cập nhật lại nfds nếu đây là phần tử cuối
                    if (i == nfds - 1) {
                        while (nfds > 1 && poll_fds[nfds - 1].fd == -1) {
                            nfds--;
                        }
                    }
                } else {
                    // Xử lý dữ liệu nhận được
                    buffer[rcvBytes] = '\0';
                    
                    // Loại bỏ newline nếu có
                    if (rcvBytes > 0 && buffer[rcvBytes - 1] == '\n') {
                        buffer[rcvBytes - 1] = '\0';
                    }
                    
                    int client_index = find_client_by_sockfd(sockfd);
                    if (client_index >= 0) {
                        handle_client_message(client_index, buffer, list, poll_fds, &nfds);
                    }
                }
            }
            
            // Kiểm tra lỗi hoặc ngắt kết nối
            if (poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                int sockfd = poll_fds[i].fd;
                int client_index = find_client_by_sockfd(sockfd);
                if (client_index >= 0) {
                    remove_client(client_index, list);
                }
                
                poll_fds[i].fd = -1;
                
                if (i == nfds - 1) {
                    while (nfds > 1 && poll_fds[nfds - 1].fd == -1) {
                        nfds--;
                    }
                }
            }
        }
    }

    // Cleanup
    close(listenfd);
    return 0;
}
