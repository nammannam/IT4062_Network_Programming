#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include "llist.h"
#include "./protocol.h"

#define INPUT_FILE_PATH "./account.txt"
#define HISTORY_FILE_PATH "./history.txt"
#define MAX_LEN 20
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

// Client state structure for non-blocking I/O
typedef struct
{
    int sockfd;
    struct sockaddr_in addr;
    int is_logged_in;
    char username[MAX_LEN];
    char current_logged_in_user[MAX_LEN]; // For logout logging
    char pending_username[64];            // Username waiting for password
    int failed_attempts;
    int active;
    char recv_buffer[sizeof(application_msg_t)];
    size_t recv_bytes;
} ClientState_s;

ClientState_s clients[MAX_CLIENTS];
int client_count = 0;

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
        *attempts = 0; // Reset attempts on successful login
        // Note: No longer set global logged in state in non-blocking mode
        return 4;
    }
}

void accountSignOut()
{
    // This function is deprecated in non-blocking mode
    // Client state management handles logout
    printf("Sign out function called (deprecated in non-blocking mode).\n");
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

void changePassword(Llist_s *list, char *username, char *newPassword, char *reply)
{
    char *replyResult;

    // Cập nhật mật khẩu trong danh sách liên kết
    Llist_s *pt;
    pt = findNodeByUsername(list, username);
    if (pt != NULL)
    {
        strcpy(pt->nodeInfo.password, newPassword);
    }
    else
    {
        printf("Error: User not found in the list.\n");
        sprintf(reply, "Error: User not found\n");
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

void sig_chld_handler(int signo)
{
    // No longer needed with non-blocking I/O, but keep for compatibility
    (void)signo;
}

// Set socket to non-blocking mode
int set_nonblocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl F_GETFL");
        return -1;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl F_SETFL");
        return -1;
    }
    return 0;
}

// Initialize a client state
void init_client_state(ClientState_s *client)
{
    client->sockfd = -1;
    memset(&client->addr, 0, sizeof(client->addr));
    client->is_logged_in = 0;
    memset(client->username, 0, MAX_LEN);
    memset(client->current_logged_in_user, 0, MAX_LEN);
    memset(client->pending_username, 0, sizeof(client->pending_username));
    client->failed_attempts = 0;
    client->active = 0;
    memset(client->recv_buffer, 0, sizeof(client->recv_buffer));
    client->recv_bytes = 0;
}

// Add a new client
int add_client(int sockfd, struct sockaddr_in addr)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (!clients[i].active)
        {
            clients[i].sockfd = sockfd;
            clients[i].addr = addr;
            clients[i].is_logged_in = 0;
            memset(clients[i].username, 0, MAX_LEN);
            memset(clients[i].current_logged_in_user, 0, MAX_LEN);
            clients[i].failed_attempts = 0;
            clients[i].active = 1;
            clients[i].recv_bytes = 0;
            memset(clients[i].recv_buffer, 0, sizeof(clients[i].recv_buffer));
            client_count++;
            printf("Client added at index %d (sockfd=%d)\n", i, sockfd);
            return i;
        }
    }
    printf("Max clients reached!\n");
    return -1;
}

// Remove a client
void remove_client(int index, SharedClientList_s *shm, Llist_s *list)
{
    if (index < 0 || index >= MAX_CLIENTS || !clients[index].active)
    {
        return;
    }

    ClientState_s *client = &clients[index];
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client->addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client->addr.sin_port);

    printf("Removing client at index %d (sockfd=%d, %s:%d)\n",
           index, client->sockfd, client_ip, client_port);

    // Log logout if user was logged in
    if (client->is_logged_in && strlen(client->current_logged_in_user) > 0)
    {
        log_logout(client->current_logged_in_user, client_ip, client_port);
    }

    // Remove from shared memory
    if (client->is_logged_in && shm != NULL)
    {
        // Use socket fd as unique identifier instead of pid
        remove_logged_in_client(shm, client->sockfd);
    }

    close(client->sockfd);
    init_client_state(client);
    client_count--;
}

// Handle client message
void handle_client_message(int client_index, Llist_s *list, SharedClientList_s *shm)
{
    ClientState_s *client = &clients[client_index];

    // Try to read a complete message
    ssize_t n = recv(client->sockfd,
                     client->recv_buffer + client->recv_bytes,
                     sizeof(application_msg_t) - client->recv_bytes,
                     0);

    if (n <= 0)
    {
        if (n == 0)
        {
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client->addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            printf("Client (%s:%d) disconnected.\n",
                   client_ip, ntohs(client->addr.sin_port));
        }
        else if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            perror("recv");
        }
        remove_client(client_index, shm, list);
        return;
    }

    client->recv_bytes += n;

    // Check if we have a complete message
    if (client->recv_bytes < sizeof(application_msg_t))
    {
        return; // Need more data
    }

    // We have a complete message
    application_msg_t *msgIn = (application_msg_t *)client->recv_buffer;
    application_msg_t msgOut;
    memset(&msgOut, 0, sizeof(msgOut));

    // Ensure value is null-terminated based on length
    if (msgIn->len < MAX_VALUE_LEN)
    {
        msgIn->value[msgIn->len] = '\0';
    }
    else
    {
        msgIn->value[MAX_VALUE_LEN - 1] = '\0';
    }

    printf("[DEBUG] Client %d - Type: 0x%02X, Len: %d, Value: '%s'\n",
           client_index, msgIn->type, msgIn->len, msgIn->value);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client->addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client->addr.sin_port);

    // Use sockfd as unique identifier instead of pid for non-blocking context
    // Pass both pending_username and username (logged_in_username) for per-client state
    server_handle_message(msgIn, &msgOut, list, client->sockfd, client_ip, client_port,
                          client->pending_username, client->username);

    // Send response
    send(client->sockfd, &msgOut, sizeof(msgOut), 0);

    // Track login status
    if (msgOut.type == MSG_CF && msgIn->type == MSG_PASSWORD)
    {
        client->is_logged_in = 1;
        strncpy(client->current_logged_in_user, msgOut.value, MAX_LEN - 1);
        client->current_logged_in_user[MAX_LEN - 1] = '\0';
    }

    // Handle bye command
    if (strcmp(msgIn->value, "bye") == 0)
    {
        remove_client(client_index, shm, list);
        return;
    }

    // Reset buffer for next message
    client->recv_bytes = 0;
    memset(client->recv_buffer, 0, sizeof(client->recv_buffer));
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Lấy server port
    char *endptr;
    long servPort_l = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || servPort_l <= 0 || servPort_l > 65535)
    {
        fprintf(stderr, "Invalid port: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    uint16_t servPort = (uint16_t)servPort_l;

    // Load account list
    Llist_s *list = newList();
    readAccountsFromFile(INPUT_FILE_PATH, list);

    // Initialize shared memory
    SharedClientList_s *shm = init_shared_memory();
    if (shm != NULL)
    {
        memset(shm, 0, sizeof(SharedClientList_s));
        printf("Shared memory initialized successfully.\n");
    }
    else
    {
        fprintf(stderr, "Warning: Failed to initialize shared memory.\n");
    }

    // Initialize client states
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        init_client_state(&clients[i]);
    }

    int listenfd;
    struct sockaddr_in servaddr;

    // Create TCP socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket to non-blocking
    if (set_nonblocking(listenfd) < 0)
    {
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    // Allow socket reuse
    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Configure server address
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(servPort);

    // Bind socket
    if (bind(listenfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(listenfd, 10) < 0)
    {
        perror("listen failed");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d (non-blocking mode)...\n", servPort);
    printf("Using select() for I/O multiplexing\n");

    // Main event loop with select()
    fd_set read_fds, master_fds;
    FD_ZERO(&master_fds);
    FD_SET(listenfd, &master_fds);
    int max_fd = listenfd;

    while (1)
    {
        read_fds = master_fds;

        printf("[DEBUG] Waiting for activity (max_fd=%d, client_count=%d)...\n", max_fd, client_count);

        // Wait for activity on any socket
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("select");
            break;
        }

        printf("[DEBUG] select() returned activity=%d\n", activity);

        // Check for new connection
        if (FD_ISSET(listenfd, &read_fds))
        {
            printf("[DEBUG] New connection detected on listenfd\n");
            struct sockaddr_in cliaddr;
            socklen_t cliaddr_len = sizeof(cliaddr);
            int connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);

            if (connfd < 0)
            {
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    perror("accept");
                }
            }
            else
            {
                printf("[DEBUG] Accepted connfd=%d\n", connfd);
                // Set new connection to non-blocking
                if (set_nonblocking(connfd) < 0)
                {
                    close(connfd);
                }
                else
                {
                    int client_idx = add_client(connfd, cliaddr);
                    if (client_idx >= 0)
                    {
                        FD_SET(connfd, &master_fds);
                        if (connfd > max_fd)
                        {
                            max_fd = connfd;
                        }
                        printf("Accepted connection from %s:%d (sockfd=%d, index=%d, total=%d)\n",
                               inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port),
                               connfd, client_idx, client_count);
                    }
                    else
                    {
                        printf("[ERROR] Failed to add client (max clients reached?)\n");
                        close(connfd);
                    }
                }
            }
        }

        // Check for client activity
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].active && FD_ISSET(clients[i].sockfd, &read_fds))
            {
                printf("[DEBUG] Activity on client %d (sockfd=%d)\n", i, clients[i].sockfd);
                int old_sockfd = clients[i].sockfd;
                handle_client_message(i, list, shm);

                // If client was removed, clear from fd_set
                if (!clients[i].active || clients[i].sockfd != old_sockfd)
                {
                    printf("[DEBUG] Client %d removed, clearing from fd_set\n", i);
                    FD_CLR(old_sockfd, &master_fds);

                    // Recalculate max_fd if needed
                    if (old_sockfd == max_fd)
                    {
                        max_fd = listenfd;
                        for (int j = 0; j < MAX_CLIENTS; j++)
                        {
                            if (clients[j].active && clients[j].sockfd > max_fd)
                            {
                                max_fd = clients[j].sockfd;
                            }
                        }
                        printf("[DEBUG] Recalculated max_fd=%d\n", max_fd);
                    }
                }
            }
        }
    }

    // Cleanup
    close(listenfd);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].active)
        {
            close(clients[i].sockfd);
        }
    }
    if (shm != NULL)
    {
        shmdt(shm);
    }
    deleteList(list);

    return 0;
}
