#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include "llist.h"

#define AUTH_LOG_FILE "./log/auth.log"

// Authentication logging functions
void log_login_success(const char *username, const char *client_ip, int client_port) {
    char *log_dir = "./log/";
    if(access(log_dir, F_OK) == -1){
        mkdir(log_dir, 0700);
    }
    
    FILE *log_file = fopen(AUTH_LOG_FILE, "a");
    if (log_file != NULL) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
        
        fprintf(log_file, "[%s] LOGIN %s from %s:%d SUCCESS\n", 
                timestamp, username, client_ip, client_port);
        fclose(log_file);
        
        printf("[AUTH LOG] Login success: %s from %s:%d\n", username, client_ip, client_port);
    }
}

void log_login_fail(const char *username, const char *client_ip, int client_port, const char *reason) {
    char *log_dir = "./log/";
    if(access(log_dir, F_OK) == -1){
        mkdir(log_dir, 0700);
    }
    
    FILE *log_file = fopen(AUTH_LOG_FILE, "a");
    if (log_file != NULL) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
        
        fprintf(log_file, "[%s] LOGIN %s from %s:%d FAIL (%s)\n", 
                timestamp, username, client_ip, client_port, reason);
        fclose(log_file);
        
        printf("[AUTH LOG] Login failed: %s from %s:%d - %s\n", username, client_ip, client_port, reason);
    }
}

void log_account_locked(const char *username) {
    char *log_dir = "./log/";
    if(access(log_dir, F_OK) == -1){
        mkdir(log_dir, 0700);
    }
    
    FILE *log_file = fopen(AUTH_LOG_FILE, "a");
    if (log_file != NULL) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
        
        fprintf(log_file, "[%s] ACCOUNT_LOCKED %s\n", timestamp, username);
        fclose(log_file);
        
        printf("[AUTH LOG] Account locked: %s\n", username);
    }
}

void log_logout(const char *username, const char *client_ip, int client_port) {
    char *log_dir = "./log/";
    if(access(log_dir, F_OK) == -1){
        mkdir(log_dir, 0700);
    }
    
    FILE *log_file = fopen(AUTH_LOG_FILE, "a");
    if (log_file != NULL) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
        
        fprintf(log_file, "[%s] LOGOUT %s from %s:%d\n", 
                timestamp, username, client_ip, client_port);
        fclose(log_file);
        
        printf("[AUTH LOG] Logout: %s from %s:%d\n", username, client_ip, client_port);
    }
}

// Tạo message với kiểu và dữ liệu cho trước
void create_message(application_msg_t *msg, msg_type_t type, const char *data)
{
    if (msg == NULL)
        return;

    size_t data_len = 0;
    if (data != NULL)
    {
        data_len = strlen(data);
    }

    if (data_len > MAX_VALUE_LEN)
    {

        data_len = MAX_VALUE_LEN;
        fprintf(stderr, "Cảnh báo: Dữ liệu bị cắt bớt do vượt quá MAX_VALUE_LENGTH (%d).\n", MAX_VALUE_LEN);
    }

    msg->type = (uint8_t)type;
    msg->len = (uint8_t)data_len;

    if (data_len > 0 && data != NULL)
    {
        // Sao chép dữ liệu vào message strncpy để tránh tràn bộ đệm
        strncpy(msg->value, data, data_len);
    }

    msg->value[data_len] = '\0'; // Đảm bảo chuỗi kết thúc đúng cách
}

// Ghi log message nhận được từ client
void server_log_msg(const char *client_usr, application_msg_t *msg, pthread_t thread_id)
{
    char *log_dir = "./log/";
    if(access(log_dir, F_OK) == -1){
        mkdir(log_dir, 0700);
    }

    if (msg == NULL || client_usr == NULL)
        return;

    printf("Received message from %s: Type=0x%02X, Length=%d, Value=\"%s\"\n",
           client_usr, msg->type, msg->len, msg->value);

    char client_file_name[64];
    // snprintf(client_file_name, sizeof(client_file_name), "./log/%s_log.txt", client_usr);
    snprintf(client_file_name, sizeof(client_file_name), "./log/user.txt");
    // Ghi log vào file
    FILE *log_file = fopen(client_file_name, "a+");
    if (log_file != NULL)
    {
        fprintf(log_file, "THREAD: %lu | Client: %s | Type=0x%02X | Length=%d | Value=\"%s\"\n",
                (unsigned long)thread_id, client_usr, msg->type, msg->len, msg->value);
        fclose(log_file);
    }
}

// REMOVED GLOBAL VARIABLES to fix race condition with multiple threads
// These are now passed as parameters to server_handle_message()
// char logged_in_client[64];        // ❌ Race condition!
// char pending_username[64];        // ❌ Race condition!

// Structure to track failed login attempts per username
#define MAX_TRACKED_USERS 50
typedef struct {
    char username[MAX_LEN];
    int failed_attempts;
} FailedAttempt_s;

FailedAttempt_s failed_attempts_tracker[MAX_TRACKED_USERS];
int tracker_count = 0;

// Get or create failed attempts entry for a username
int* get_failed_attempts(const char *username) {
    // Search for existing entry
    for (int i = 0; i < tracker_count; i++) {
        if (strcmp(failed_attempts_tracker[i].username, username) == 0) {
            return &failed_attempts_tracker[i].failed_attempts;
        }
    }
    
    // Create new entry if not found and space available
    if (tracker_count < MAX_TRACKED_USERS) {
        strncpy(failed_attempts_tracker[tracker_count].username, username, MAX_LEN - 1);
        failed_attempts_tracker[tracker_count].username[MAX_LEN - 1] = '\0';
        failed_attempts_tracker[tracker_count].failed_attempts = 0;
        return &failed_attempts_tracker[tracker_count++].failed_attempts;
    }
    
    return NULL;
}

// Reset failed attempts for a username
void reset_failed_attempts(const char *username) {
    for (int i = 0; i < tracker_count; i++) {
        if (strcmp(failed_attempts_tracker[i].username, username) == 0) {
            failed_attempts_tracker[i].failed_attempts = 0;
            return;
        }
    }
}

// Lock account by changing status from '1' to '0' in file
void lock_account_in_file(const char *username, const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        perror("Cannot open account file for reading");
        return;
    }
    
    // Read all accounts into memory
    char lines[100][256];
    int line_count = 0;
    
    while (fgets(lines[line_count], sizeof(lines[0]), file) != NULL && line_count < 100) {
        line_count++;
    }
    fclose(file);
    
    // Update the account status
    for (int i = 0; i < line_count; i++) {
        char file_username[MAX_LEN];
        char file_password[MAX_LEN];
        char file_email[MAX_LEN];
        char file_homepage[MAX_LEN];
        char file_status;
        
        if (sscanf(lines[i], "%s %s %s %s %c", file_username, file_password, 
                   file_email, file_homepage, &file_status) == 5) {
            if (strcmp(file_username, username) == 0) {
                // Lock the account by changing status to '0'
                snprintf(lines[i], sizeof(lines[0]), "%s %s %s %s 0\n", 
                        file_username, file_password, file_email, file_homepage);
                printf("[LOCK ACCOUNT] Locked account: %s (changed status to 0)\n", username);
                break;
            }
        }
    }
    
    // Write back to file
    file = fopen(filepath, "w");
    if (file == NULL) {
        perror("Cannot open account file for writing");
        return;
    }
    
    for (int i = 0; i < line_count; i++) {
        fputs(lines[i], file);
    }
    fclose(file);
}

// Lock account in the in-memory linked list
void lock_account_in_list(Llist_s *list, const char *username) {
    if (list == NULL || username == NULL) {
        return;
    }

    Llist_s *current = list;
    while (current != NULL) {
        if (strcmp(current->nodeInfo.username, username) == 0) {
            // Update status to '0' (locked)
            current->nodeInfo.status = '0';
            printf("[LOCK ACCOUNT IN LIST] Locked account in memory: %s (status = 0)\n", username);
            return;
        }
        current = current->next;
    }
    
    printf("[LOCK ACCOUNT IN LIST] Account not found in list: %s\n", username);
}

// Initialize client list with mutex
void init_client_list(SharedClientList_s *list) {
    if (list == NULL) return;
    
    memset(list->clients, 0, sizeof(list->clients));
    list->count = 0;
    pthread_mutex_init(&list->mutex, NULL);
    printf("[CLIENT LIST] Initialized with mutex\n");
}

// Add a logged-in client (thread-safe)
void add_logged_in_client(SharedClientList_s *list, const char *username, pthread_t thread_id) {
    if (list == NULL || username == NULL) return;
    
    pthread_mutex_lock(&list->mutex);
    
    // Check if client already exists (by thread_id)
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (list->clients[i].active && list->clients[i].thread_id == thread_id) {
            // Update existing entry
            strncpy(list->clients[i].username, username, MAX_LEN - 1);
            list->clients[i].username[MAX_LEN - 1] = '\0';
            pthread_mutex_unlock(&list->mutex);
            return;
        }
    }
    
    // Find empty slot
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!list->clients[i].active) {
            strncpy(list->clients[i].username, username, MAX_LEN - 1);
            list->clients[i].username[MAX_LEN - 1] = '\0';
            list->clients[i].thread_id = thread_id;
            list->clients[i].active = 1;
            list->count++;
            printf("[CLIENT LIST] Added client: %s (Thread: %lu)\n", username, (unsigned long)thread_id);
            pthread_mutex_unlock(&list->mutex);
            return;
        }
    }
    
    fprintf(stderr, "[CLIENT LIST] Warning: Client list full!\n");
    pthread_mutex_unlock(&list->mutex);
}

// Remove a logged-in client (thread-safe)
void remove_logged_in_client(SharedClientList_s *list, pthread_t thread_id) {
    if (list == NULL) return;
    
    pthread_mutex_lock(&list->mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (list->clients[i].active && list->clients[i].thread_id == thread_id) {
            printf("[CLIENT LIST] Removed client: %s (Thread: %lu)\n", 
                   list->clients[i].username, (unsigned long)thread_id);
            list->clients[i].active = 0;
            memset(list->clients[i].username, 0, MAX_LEN);
            list->clients[i].thread_id = 0;
            list->count--;
            pthread_mutex_unlock(&list->mutex);
            return;
        }
    }
    
    pthread_mutex_unlock(&list->mutex);
}

// Get list of logged-in clients as a formatted string (thread-safe)
void get_logged_in_clients_list(SharedClientList_s *list, char *output_buffer, size_t buffer_size) {
    if (list == NULL || output_buffer == NULL) return;
    
    pthread_mutex_lock(&list->mutex);
    
    memset(output_buffer, 0, buffer_size);
    
    if (list->count == 0) {
        snprintf(output_buffer, buffer_size, "No users currently logged in.");
        pthread_mutex_unlock(&list->mutex);
        return;
    }
    
    char temp[MAX_VALUE_LEN];
    int first = 1;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (list->clients[i].active) {
            if (first) {
                snprintf(temp, sizeof(temp), "%s", list->clients[i].username);
                first = 0;
            } else {
                snprintf(temp, sizeof(temp), ", %s", list->clients[i].username);
            }
            
            // Ensure we don't overflow the output buffer
            if (strlen(output_buffer) + strlen(temp) < buffer_size - 1) {
                strcat(output_buffer, temp);
            }
        }
    }
    
    pthread_mutex_unlock(&list->mutex);
}

// Triển khai logic xử lý ở phía Server
void server_handle_message(application_msg_t *in_msg, application_msg_t *out_msg, Llist_s *client_list, pthread_t thread_id, const char *client_ip, int client_port, SharedClientList_s *shared_clients, char *logged_in_username, char *pending_username) {
    // Luôn đảm bảo chuỗi trong in_msg kết thúc null trước khi sử dụng
    printf("DEBUG: len = %d\n", in_msg->len);
    in_msg->value[in_msg->len] = '\0'; 
    const char* received_value = in_msg->value;

    switch (in_msg->type) {
        case MSG_LOGIN: {
            printf("[SERVER]|[THREAD %lu] Nhận yêu cầu LOGIN: %s\n", (unsigned long)thread_id, received_value);
            
            // 1. Kiểm tra Login Name có tồn tại trong account.txt
            if (in_msg->len > 0) {
                AccountInfo_s found_acc = searchUsernameList(client_list, in_msg->value);
                
                if(client_list != NULL && strcmp(found_acc.username, "") != 0){
                    // Tài khoản tồn tại - KIỂM TRA STATUS TRƯỚC
                    
                    // 2. Kiểm tra tài khoản có bị khóa không (status == '0')
                    if (found_acc.status == '0') {
                        // Tài khoản bị khóa - Từ chối ngay, không cho nhập password
                        create_message(out_msg, MSG_DENY, "Account is blocked.");
                        printf("[SERVER]|[THREAD %lu] Tài khoản bị khóa: %s\n", (unsigned long)thread_id, in_msg->value);
                        
                        // Log failed login
                        log_login_fail(in_msg->value, client_ip, client_port, "account blocked");
                    } else {
                        // Tài khoản active - Lưu username và yêu cầu password
                        strncpy(pending_username, received_value, sizeof(pending_username) - 1);
                        pending_username[sizeof(pending_username) - 1] = '\0';
                        
                        // Yêu cầu client gửi password
                        create_message(out_msg, MSG_PASSWORD, "Enter password:");
                        printf("[SERVER]|[THREAD %lu] Gửi yêu cầu nhập password cho %s\n", (unsigned long)thread_id, pending_username);
                    }

                } else if(client_list != NULL){
                    // Tài khoản không tồn tại
                    create_message(out_msg, MSG_DENY, "Account does not exist.");
                    printf("[SERVER]|[THREAD %lu] Gửi phản hồi: DENY (tài khoản không tồn tại)\n", (unsigned long)thread_id);
                }
            } else {
                // Tên rỗng
                create_message(out_msg, MSG_DENY, "Login name cannot be empty.");
                printf("[SERVER]|[THREAD %lu] Gửi phản hồi: DENY (tên rỗng)\n", (unsigned long)thread_id);
            }
            break;
        }

        case MSG_PASSWORD: {
            // Xử lý password từ client
            if (strlen(pending_username) > 0) {
                printf("[SERVER]|[THREAD %lu] Nhận password cho username: %s\n", (unsigned long)thread_id, pending_username);
                
                // Kiểm tra password
                AccountInfo_s acc = searchUsernameList(client_list, pending_username);
                
                if (strcmp(acc.password, received_value) == 0) {
                    // Password đúng
                    if (acc.status == '1') {
                        // Tài khoản active - Đăng nhập thành công
                        strncpy(logged_in_username, pending_username, MAX_LEN - 1);
                        logged_in_username[MAX_LEN - 1] = '\0';
                        
                        // Reset failed attempts counter on successful login
                        reset_failed_attempts(pending_username);
                        
                        // Thêm vào shared client list
                        add_logged_in_client(shared_clients, logged_in_username, (unsigned long)thread_id);
                        
                        create_message(out_msg, MSG_CF, logged_in_username);
                        printf("[SERVER]|[THREAD %lu] Đăng nhập thành công: %s\n", (unsigned long)thread_id, logged_in_username);
                        
                        // Log login success
                        log_login_success(logged_in_username, client_ip, client_port);
                        
                        // Xóa pending username
                        memset(pending_username, 0, MAX_LEN);
                    } else {
                        // Tài khoản bị khóa (status == '0')
                        create_message(out_msg, MSG_DENY, "Account is blocked.");
                        printf("[SERVER]|[THREAD %lu] Tài khoản bị khóa: %s\n", (unsigned long)thread_id, pending_username);
                        
                        // Log account locked
                        log_account_locked(pending_username);
                        log_login_fail(pending_username, client_ip, client_port, "account locked");
                        
                        memset(pending_username, 0, MAX_LEN);
                    }
                } else {
                    // Password sai - Increment failed attempts
                    int *attempts = get_failed_attempts(pending_username);
                    
                    if (attempts != NULL) {
                        (*attempts)++;
                        printf("[SERVER]|[THREAD %lu] Password sai cho: %s (Attempts: %d/3)\n", 
                               (unsigned long)thread_id, pending_username, *attempts);
                        
                        if (*attempts >= 3) {
                            // Lock account after 3 failed attempts
                            lock_account_in_file(pending_username, "./account.txt");
                            lock_account_in_list(client_list, pending_username);
                            
                            create_message(out_msg, MSG_DENY, "Account locked due to too many failed login attempts.");
                            printf("[SERVER]|[THREAD %lu] Tài khoản bị khóa do đăng nhập sai quá 3 lần: %s\n", 
                                   (unsigned long)thread_id, pending_username);
                            
                            // Log account locked
                            log_account_locked(pending_username);
                            log_login_fail(pending_username, client_ip, client_port, "too many failed attempts");
                            
                            // Reset counter
                            reset_failed_attempts(pending_username);
                        } else {
                            // Still have attempts left
                            char fail_msg[MAX_VALUE_LEN];
                            snprintf(fail_msg, sizeof(fail_msg), 
                                    "Incorrect password. Attempt %d/3. Warning: Account will be locked after 3 failed attempts.", 
                                    *attempts);
                            create_message(out_msg, MSG_DENY, fail_msg);
                            
                            // Log login failure
                            log_login_fail(pending_username, client_ip, client_port, "wrong password");
                        }
                    } else {
                        // Fallback if tracking fails
                        create_message(out_msg, MSG_DENY, "Incorrect password.");
                        log_login_fail(pending_username, client_ip, client_port, "wrong password");
                    }
                    
                    memset(pending_username, 0, MAX_LEN); // Reset để đăng nhập lại
                }
            } else {
                // Chưa gửi username trước đó
                create_message(out_msg, MSG_DENY, "Please login with username first.");
                printf("[SERVER]|[THREAD %lu] Chưa có username\n", (unsigned long)thread_id);
            }
            break;
        }

        case MSG_TEXT: {
            // Chỉ xử lý tin nhắn TEXT nếu Client đã đăng nhập [2]
            if (strlen(logged_in_username) > 0) {
                printf("[SERVER]|[THREAD %lu] Nhận TEXT từ [%s] : %s\n", (unsigned long)thread_id, logged_in_username, received_value);
                
                // 1. Lưu dữ liệu Text message vào log riêng của Client [1, 2]
                server_log_msg(logged_in_username, in_msg, (unsigned long)thread_id);
                
                // 2. Gửi lại Client confirm message [2]
                create_message(out_msg, MSG_CF, logged_in_username);
                printf("[SERVER]|[THREAD %lu] Gửi phản hồi: CONFIRM sau khi lưu log \n", (unsigned long)thread_id);
            } else {
                // Trường hợp Client gửi TEXT mà chưa LOGIN
                create_message(out_msg, MSG_DENY, "Error: Must login first (MSG_LOGIN required).");
                printf("[SERVER]|[THREAD %lu] Gửi phản hồi: DENY (chưa đăng nhập) \n", (unsigned long)thread_id);
            }
            break;
        }
        
        case MSG_WHO: {
            // Xử lý yêu cầu WHO - trả về danh sách các client đã đăng nhập
            if (strlen(logged_in_username) > 0) {
                printf("[SERVER]|[THREAD %lu] Nhận yêu cầu WHO từ [%s]\n", (unsigned long)thread_id, logged_in_username);
                
                // Lấy danh sách từ shared client list
                char client_list_str[MAX_VALUE_LEN];
                get_logged_in_clients_list(shared_clients, client_list_str, sizeof(client_list_str));
                
                // Gửi phản hồi MSG_LIST với danh sách clients
                create_message(out_msg, MSG_LIST, client_list_str);
                printf("[SERVER]|[THREAD %lu] Gửi danh sách: %s\n", (unsigned long)thread_id, client_list_str);
            } else {
                // Trường hợp Client gửi WHO mà chưa LOGIN
                create_message(out_msg, MSG_DENY, "Error: Must login first (MSG_LOGIN required).");
                printf("[SERVER]|[THREAD %lu] Gửi phản hồi: DENY (chưa đăng nhập) \n", (unsigned long)thread_id);
            }
            break;
        }
        
        case MSG_HELP: {
            // Xử lý yêu cầu HELP - trả về danh sách các lệnh được hỗ trợ
            if (strlen(logged_in_username) > 0) {
                printf("[SERVER]|[THREAD %lu] Nhận yêu cầu HELP từ [%s]\n", (unsigned long)thread_id, logged_in_username);
                
                char help_message[MAX_VALUE_LEN];
                snprintf(help_message, sizeof(help_message),
                    "\n=== AVAILABLE COMMANDS ===\n"
                    "who  - Display list of logged-in users\n"
                    "help - Display this help message\n"
                    "bye  - Logout and disconnect\n"
                    "==========================");
                
                // Gửi phản hồi MSG_CF với danh sách lệnh
                create_message(out_msg, MSG_CF, help_message);
                printf("[SERVER]|[THREAD %lu] Gửi HELP message\n", (unsigned long)thread_id);
            } else {
                // Trường hợp Client gửi HELP mà chưa LOGIN
                create_message(out_msg, MSG_DENY, "Error: Must login first (MSG_LOGIN required).");
                printf("[SERVER]|[THREAD %lu] Gửi phản hồi: DENY (chưa đăng nhập) \n", (unsigned long)thread_id);
            }
            break;
        }
        
        default: {
            // Xử lý thông điệp không xác định
            create_message(out_msg, MSG_DENY, "Unknown message type.");
            break;
        }
    }
}

// Triển khai logic xử lý ở phía Client
void client_handle_response(application_msg_t *in_msg) {
    in_msg->value[in_msg->len] = '\0'; 
    const char* recv_value = in_msg->value;

    switch (in_msg->type) {
        case MSG_CF: {
            printf("[CLIENT] Server CONFIRM (0x%02X): %s\n", in_msg->type, recv_value);
            // Client nhận Confirm message -> Có thể gửi tiếp text message tới Server [2]
            break;

        }
        case MSG_PASSWORD: {
            printf("[CLIENT] %s", recv_value);
            // Server yêu cầu nhập password
            break;
        }
        case MSG_DENY: {
            printf("[CLIENT] Server DENY (0x%02X): ERROR: %s\n", in_msg->type, recv_value);
            // Client nhận Deny message -> Thực hiện lại gửi login message cho Server [2]
            break;
            
        }
        case MSG_LIST: {
            printf("[CLIENT] Logged-in users: %s\n", recv_value);
            break;
        }
        default: {
            printf("[CLIENT] Phản hồi không mong muốn (0x%02X).\n", in_msg->type);
            break;

        }
    }
}
