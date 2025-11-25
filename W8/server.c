#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "llist.h"
#include "./protocol.h"

#define INPUT_FILE_PATH "./account.txt"
#define HISTORY_FILE_PATH "./history.txt"
#define MAX_LEN 20
#define BUFFER_SIZE 1024

AccountInfo_s loggedInUser; // Current logged-in user
int isLoggedIn = 0;         // 0 - no user logged in, 1 - user logged in

// Global shared client list (thread-safe with mutex)
SharedClientList_s global_client_list;

// Thread argument structure
typedef struct {
    int connfd;
    struct sockaddr_in cliaddr;
    Llist_s *account_list;
} thread_arg_t;

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

void updateAccountListFromFile(char *filePath, Llist_s *list)
{
    FILE *fptr = fopen(filePath, "r");
    if (fptr == NULL)
    {
        printf("File %s could not be opened for reading!\n", filePath);
        exit(EXIT_FAILURE);
    }
    AccountInfo_s acc;
    list->next = NULL; // Clear existing list
    while (fscanf(fptr, "%s %s %s %s %c\n", acc.username, acc.password, acc.email, acc.homepage, &acc.status) != EOF)
    {
        list = insertAtTail(list, acc);
    }

    fclose(fptr);
}

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

AccountInfo_s searchAccountInFile(char *filePath, char *username)
{
    AccountInfo_s acc = {"", "", "", "", '0'};
    FILE *fptr = fopen(filePath, "r");
    if (fptr == NULL)
    {
        printf("File %s could not be opened for reading!\n", filePath);
        return acc;
    }
    char fileUsername[MAX_LEN];
    char filePassword[MAX_LEN];
    char fileEmail[MAX_LEN];
    char fileHomepage[MAX_LEN];
    char fileStatus;

    while (fscanf(fptr, "%s %s %s %s %c\n", fileUsername, filePassword, fileEmail, fileHomepage, &fileStatus) != EOF)
    {
        if (strcmp(fileUsername, username) == 0)
        {
            strcpy(acc.username, fileUsername);
            strcpy(acc.password, filePassword);
            strcpy(acc.email, fileEmail);
            strcpy(acc.homepage, fileHomepage);
            acc.status = fileStatus;
            fclose(fptr);

            return acc;
        }
    }

    fclose(fptr);
    return acc;
}

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
    while ((pos = ftell(fptr)) != -1 && fscanf(fptr, "%s %s %s %s %c\n", fileUsername, filePassword, fileEmail, fileHomepage, &fileStatus) != EOF) // Đọc check cả \n
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

int accountSignIn(Llist_s *list, char *filePath, char *username, char *password, int *attempts)
{
    AccountInfo_s searchAcc = searchUsernameList(list, username);
    AccountInfo_s resultAcc;

    // Không tìm thấy Account -> Trả về struct rỗng -> 0
    if (strcmp(searchAcc.username, "") == 0)
    {
        printf("Cannot find account\n");
        return 0;
    }

    // Account bị khóa -> Trả về chỉ status = '0' -> 1
    if (searchAcc.status == '0')
    {
        printf("Account is blocked\n");
        return 1;
    }

    if (strcmp(searchAcc.password, password) != 0)
    {
        printf("Incorrect password\n");
        (*attempts)++;

        // Khóa account sau 3 lần nhập sai password -> trả về username và status = 0 -> 2
        if (*attempts >= 3)
        {
            printf("Account is blocked due to 3 failed login attempts\n");
            // Update the account status in the file to '0' (blocked)
            updateAccountStatusInFile(filePath, username, '0');
            updateAccountListFromFile(filePath, list); // Cập nhật lại danh sách liên kết

            return 2;
        }

        // Sai mật khẩu Account nhưng còn attempts -> Trả về chỉ username -> 3
        printf("You have %d attempt(s) left\n", 3 - *attempts);

        return 3;
    }
    else
    {
        // Đăng nhập thành công -> Trả về đầy đủ thông tin Account đã đăng nhập -> 4
        *attempts = 0;            // Reset attempts on successful login
        loggedInUser = searchAcc; // Cập nhật thông tin người dùng đã đăng nhập
        isLoggedIn = 1;           // Cập nhật trạng thái đã đăng nhập
        return 4;
    }
}

void accountSignOut()
{
    if (!isLoggedIn)
    {
        printf("No user is currently logged in.\n");
        return;
    }

    printf("User %s signed out successfully.\n", loggedInUser.username);
    isLoggedIn = 0;
    memset(&loggedInUser, 0, sizeof(loggedInUser));
}

char *stringProcess(char *str)
{

    char *temp1 = malloc(strlen(str) + 1);
    char *temp2 = malloc(strlen(str) + 1);
    char *res = malloc(strlen(str) + 1);
    memset(temp1, 0, strlen(str) + 1);
    memset(temp2, 0, strlen(str) + 1);
    memset(res, 0, strlen(str) + 1);

    for (int i = 0; i < strlen(str); i++)
    {
        if (str[i] >= '0' && str[i] <= '9')
        {
            strncat(temp1, &str[i], 1);
        }
        if ((str[i] >= 'a' && str[i] <= 'z') || (str[i] >= 'A' && str[i] <= 'Z'))
        {
            strncat(temp2, &str[i], 1);
        }
    }
    if (strlen(temp1) == 0)
    {
        sprintf(res, "\n%s\n", temp2);
        free(temp1);
        free(temp2);
    }
    else if (strlen(temp2) == 0)
    {
        sprintf(res, "\n%s\n", temp1);
        free(temp1);
        free(temp2);
    }
    else
    {
        sprintf(res, "\n%s\n%s\n", temp1, temp2);
        free(temp1);
        free(temp2);
    }

    return res;
}

int stringCheck(char *str)
{

    int flag = 1;

    for (int i = 0; i < strlen(str); i++)
    {

        if (!(str[i] >= 'a' && str[i] <= 'z') && !(str[i] >= 'A' && str[i] <= 'Z') && !(str[i] >= '0' && str[i] <= '9'))
        {
            flag = 0;
            break;
        }
    }

    if (flag)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void changePassword(Llist_s *list, char *newPassword, char *reply)
{
    char *replyResult;

    // // Nhập mật khẩu mới - tự động loại bỏ tất cả dấu cách
    // getInputNoSpaces("Enter new password: ", newPassword, MAX_LEN);

    // Cập nhật mật khẩu trong danh sách liên kết
    Llist_s *pt;
    pt = findNodeByUsername(list, loggedInUser.username);
    if (pt != NULL)
    {
        strcpy(pt->nodeInfo.password, newPassword);
        loggedInUser = pt->nodeInfo; // Cập nhật thông tin người dùng đã đăng nhập
    }
    else
    {
        printf("Error: Logged in user not found in the list.\n");
        return;
    }

    if (stringCheck(newPassword) == 0)
    {
        sprintf(reply, "Error\n");
        return;
    }
    // Cập nhật mật khẩu trong file
    updateAccountToFile(INPUT_FILE_PATH, list);

    replyResult = stringProcess(newPassword);

    sprintf(reply, "%s", replyResult);
    free(replyResult);
    printf("Password changed successfully.\n");
}

void *handle_client_thread(void *arg)
{
    // Extract arguments
    thread_arg_t *thread_arg = (thread_arg_t *)arg;
    int connfd = thread_arg->connfd;
    struct sockaddr_in cliaddr = thread_arg->cliaddr;
    Llist_s *list = thread_arg->account_list;
    pthread_t thread_id = pthread_self();
    
    // Free thread argument after extracting data
    free(thread_arg);
    
    char username[MAX_LEN];
    char password[MAX_LEN];
    int attempts = 0;
    memset(username, 0, sizeof(username));
    memset(password, 0, sizeof(password));
    AccountInfo_s accountInfo;
    
    char client_ip[INET_ADDRSTRLEN];
    strncpy(client_ip, inet_ntoa(cliaddr.sin_addr), INET_ADDRSTRLEN - 1);
    client_ip[INET_ADDRSTRLEN - 1] = '\0';
    int client_port = ntohs(cliaddr.sin_port);

    printf("[THREAD %lu] New client connected from %s:%d\n", 
           (unsigned long)thread_id, client_ip, client_port);

    // Khai báo biến cho message
    application_msg_t msgIn;  // Nhận message từ Client
    application_msg_t msgOut; // Gửi message đến Client
    memset(&msgOut, 0, sizeof(msgOut));
    memset(&msgIn, 0, sizeof(msgIn));
    
    // Thread-local buffers to avoid race condition (each thread has its own copy)
    char logged_in_username[MAX_LEN];
    char pending_username[MAX_LEN];
    memset(logged_in_username, 0, sizeof(logged_in_username));
    memset(pending_username, 0, sizeof(pending_username));
    
    // Track logged-in username for logout logging
    char current_logged_in_user[MAX_LEN];
    memset(current_logged_in_user, 0, sizeof(current_logged_in_user));

    while (1)
    {
        int rcvBytes = recv(connfd, &msgIn, sizeof(msgIn), 0);

        if (rcvBytes < 0)
        {
            perror("Error: ");
            
            // Log logout if user was logged in
            if (isLoggedIn && strlen(current_logged_in_user) > 0) {
                log_logout(current_logged_in_user, client_ip, client_port);
            }
            
            // Xóa client khỏi global list nếu đã đăng nhập
            if (isLoggedIn) {
                remove_logged_in_client(&global_client_list, thread_id);
            }
            break;
        }
        else if (rcvBytes == 0)
        {
            printf("[THREAD %lu] Client (%s:%d) disconnected unexpectedly.\n", 
                   (unsigned long)thread_id, client_ip, client_port);

            // Log logout if user was logged in
            if (isLoggedIn && strlen(current_logged_in_user) > 0) {
                log_logout(current_logged_in_user, client_ip, client_port);
            }

            // Xóa client khỏi global list
            if (isLoggedIn) {
                remove_logged_in_client(&global_client_list, thread_id);
            }

            accountSignOut();
            memset(username, 0, sizeof(username));
            memset(password, 0, sizeof(password));
            attempts = 0;
            isLoggedIn = 0;
            break;
        }

        server_handle_message(&msgIn, &msgOut, list, thread_id, client_ip, client_port, &global_client_list, logged_in_username, pending_username);

        // Gửi response về client
        send(connfd, &msgOut, sizeof(msgOut), 0);
        
        // Track login status for logout logging
        if (msgOut.type == MSG_CF && msgIn.type == MSG_PASSWORD) {
            // User just logged in successfully
            strncpy(current_logged_in_user, msgOut.value, MAX_LEN - 1);
            current_logged_in_user[MAX_LEN - 1] = '\0';
        }

        // Xử lý cleanup khi client gửi "bye"
        if (strcmp(msgIn.value, "bye") == 0)
        {
            printf("[THREAD %lu] Client (%s:%d) sent bye command.\n", 
                   (unsigned long)thread_id, client_ip, client_port);
            
            // Log logout if user was logged in
            if (strlen(current_logged_in_user) > 0) {
                log_logout(current_logged_in_user, client_ip, client_port);
            }
            
            // Xóa client khỏi global list
            remove_logged_in_client(&global_client_list, thread_id);
            
            accountSignOut();
            memset(username, 0, sizeof(username));
            memset(password, 0, sizeof(password));
            attempts = 0;
            isLoggedIn = 0;
            memset(current_logged_in_user, 0, sizeof(current_logged_in_user));
        }
    }
    
    // Cleanup when closing connection
    close(connfd);
    printf("[THREAD %lu] Connection closed.\n", (unsigned long)thread_id);
    
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char username[MAX_LEN];
    char password[MAX_LEN];
    int attempts = 0;
    memset(username, 0, sizeof(username));
    memset(password, 0, sizeof(password));

    // Lấy server port
    char *endptr;
    long servPort_l = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || servPort_l <= 0 || servPort_l > 65535)
    {
        fprintf(stderr, "Invalid port: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    uint16_t servPort = (uint16_t)servPort_l;

    Llist_s *list = newList();
    readAccountsFromFile(INPUT_FILE_PATH, list);

    // Khởi tạo global client list với mutex
    init_client_list(&global_client_list);
    printf("Global client list initialized successfully.\n");

    int listenfd;
    int connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t cliaddr_len = sizeof(cliaddr);

    // Tạo socket TCP
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Cấu hình Server TCP
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(servPort);

    // Gán IP và Port cho Socket
    if (bind(listenfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    // Chuyển sang trạng thái lắng nghe kết nối từ Client (max 10 kết nối chờ)
    if (listen(listenfd, 10) < 0)
    {
        perror("listen failed");
        close(listenfd);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Server is listening on port %d (using pthread)...\n", servPort);
    }

    for (;;)
    {
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
        if (connfd < 0)
        {
            perror("accept failed");
            continue;
        }
        else
        {
            printf("Accepted connection from client (%s:%d)\n", 
                   inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
        }

        // Tạo thread argument
        thread_arg_t *thread_arg = (thread_arg_t *)malloc(sizeof(thread_arg_t));
        if (thread_arg == NULL) {
            perror("Failed to allocate thread argument");
            close(connfd);
            continue;
        }
        
        thread_arg->connfd = connfd;
        thread_arg->cliaddr = cliaddr;
        thread_arg->account_list = list;

        // Tạo thread mới để xử lý client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client_thread, (void *)thread_arg) != 0) {
            perror("Failed to create thread");
            free(thread_arg);
            close(connfd);
            continue;
        }

        // Detach thread để tự động cleanup khi kết thúc
        pthread_detach(thread_id);
    }

    // Cleanup
    pthread_mutex_destroy(&global_client_list.mutex);
    close(listenfd);
    return 0;
}