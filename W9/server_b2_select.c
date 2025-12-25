#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/select.h>
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
void handle_client_message(int client_index, char *buffer, Llist_s *list, fd_set *master_fds);

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
            return i;
        }
    }
    printf("Error: Maximum clients reached\n");
    return -1;
}

// Xóa client khỏi mảng
void remove_client(int index, Llist_s *list) {
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
            printf("Account is blocked due to 3 failed login attempts\n");
            updateAccountStatusInFile(filePath, username, '0');
            updateAccountListFromFile(filePath, list);
            
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
void handle_client_message(int client_index, char *buffer, Llist_s *list, fd_set *master_fds) {
    ClientState_s *client = &clients[client_index];
    char reply[BUFFER_SIZE];
    memset(reply, 0, sizeof(reply));
    
    // Xử lý lệnh "bye"
    if (strcmp(buffer, "bye") == 0 && client->is_logged_in) {
        accountSignOut(client);
        strcpy(reply, "Goodbye HUST\n");
        send(client->sockfd, reply, strlen(reply), 0);
        
        // Ngắt kết nối client
        FD_CLR(client->sockfd, master_fds);
        remove_client(client_index, list);
        return;
    }
    
    // Client chưa đăng nhập
    if (!client->is_logged_in) {
        if (client->pending_username[0] == '\0') {
            // Nhận username
            strncpy(client->pending_username, buffer, MAX_LEN - 1);
            client->pending_username[MAX_LEN - 1] = '\0';
            printf("Client %d - Username received: %s\n", client_index, client->pending_username);
            
            strcpy(reply, "Insert password:\n");
            send(client->sockfd, reply, strlen(reply), 0);
        } 
        else {
            // Nhận password
            printf("Client %d - Password received for: %s\n", client_index, client->pending_username);
            
            int result = accountSignIn(list, INPUT_FILE_PATH, 
                                      client->pending_username, 
                                      buffer, 
                                      &client->attempts,
                                      client);
            
            switch(result) {
                case 0:  // Account không tồn tại
                    strcpy(reply, "Account not found\n");
                    memset(client->pending_username, 0, MAX_LEN);
                    break;
                    
                case 1:  // Account bị khóa
                    strcpy(reply, "Account is blocked\n");
                    memset(client->pending_username, 0, MAX_LEN);
                    break;
                    
                case 2:  // Bị khóa do nhập sai 3 lần
                    strcpy(reply, "Account is blocked due to 3 failed login attempts\n");
                    memset(client->pending_username, 0, MAX_LEN);
                    client->attempts = 0;
                    break;
                    
                case 3:  // Sai mật khẩu nhưng còn attempts
                    sprintf(reply, "You have %d attempt(s) left\n", 3 - client->attempts);
                    memset(client->pending_username, 0, MAX_LEN);
                    break;
                    
                case 4:  // Đăng nhập thành công
                    strcpy(reply, "OK\n");
                    memset(client->pending_username, 0, MAX_LEN);
                    printf("Client %d logged in as: %s\n", client_index, client->username);
                    break;
            }
            
            send(client->sockfd, reply, strlen(reply), 0);
        }
    }
    // Client đã đăng nhập
    else {
        if (strcmp(buffer, "homepage") == 0) {
            // Trả về homepage
            AccountInfo_s acc = searchUsernameList(list, client->username);
            if (strcmp(acc.username, "") != 0) {
                sprintf(reply, "%s\n", acc.homepage);
            } else {
                strcpy(reply, "Error: User not found\n");
            }
            send(client->sockfd, reply, strlen(reply), 0);
        } 
        else {
            // Đổi password
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
        perror("setsockopt");
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
    printf("Using select() for I/O multiplexing\n");
    printf("Max clients: %d\n", MAX_CLIENTS);
    printf("========================================\n");

    // Khởi tạo fd_set cho select()
    fd_set master_fds, read_fds;
    FD_ZERO(&master_fds);
    FD_SET(listenfd, &master_fds);
    int max_fd = listenfd;

    // ===== VÒNG LẶP CHÍNH VỚI SELECT() =====
    while(1) {
        read_fds = master_fds;  // Copy master set
        
        // Chờ sự kiện trên các file descriptors
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        
        if (activity < 0) {
            if (errno == EINTR) continue;  // Bị ngắt bởi signal, tiếp tục
            perror("select error");
            break;
        }

        // Kiểm tra listening socket có kết nối mới không
        if (FD_ISSET(listenfd, &read_fds)) {
            struct sockaddr_in cliaddr;
            socklen_t cliaddr_len = sizeof(cliaddr);
            
            int connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
            
            if (connfd >= 0) {
                // Thiết lập socket mới non-blocking
                set_nonblocking(connfd);
                
                // Thêm client vào mảng
                int index = add_client(connfd, cliaddr);
                if (index >= 0) {
                    // Thêm vào fd_set
                    FD_SET(connfd, &master_fds);
                    if (connfd > max_fd) {
                        max_fd = connfd;
                    }
                    
                    printf("[+] New connection from %s:%d (Client #%d, Total: %d)\n", 
                           inet_ntoa(cliaddr.sin_addr), 
                           ntohs(cliaddr.sin_port),
                           index,
                           client_count);
                } else {
                    // Không còn slot trống
                    close(connfd);
                    printf("[-] Connection rejected: max clients reached\n");
                }
            }
        }

        // Kiểm tra tất cả client sockets có dữ liệu không
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) continue;
            
            if (FD_ISSET(clients[i].sockfd, &read_fds)) {
                char buffer[BUFFER_SIZE];
                int n = recv(clients[i].sockfd, buffer, BUFFER_SIZE - 1, 0);
                
                if (n <= 0) {
                    // Client ngắt kết nối hoặc lỗi
                    if (n == 0) {
                        printf("[-] Client #%d disconnected\n", i);
                    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("recv error");
                    }
                    
                    FD_CLR(clients[i].sockfd, &master_fds);
                    remove_client(i, list);
                    continue;
                }
                
                buffer[n] = '\0';
                printf("[Client #%d] Received: %s\n", i, buffer);
                
                // Xử lý message từ client
                handle_client_message(i, buffer, list, &master_fds);
            }
        }
    }

    // Cleanup
    close(listenfd);
    return 0;
}
