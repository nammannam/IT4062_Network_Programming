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
#include <pthread.h>
#include "llist.h"

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
        fprintf(log_file, "THREAD %lu: | Client: %s | Type=0x%02X | Length=%d | Value=\"%s\"\n",
                (unsigned long)thread_id, client_usr, msg->type, msg->len, msg->value);
        fclose(log_file);
    }
}

// REMOVED GLOBAL VARIABLE: char logged_in_client[64];
// Moved to thread-local parameter in server_handle_message

// Triển khai logic xử lý ở phía Server
void server_handle_message(application_msg_t *in_msg, application_msg_t *out_msg, Llist_s *client_list, pthread_t thread_id, char *logged_in_username) {
    // Luôn đảm bảo chuỗi trong in_msg kết thúc null trước khi sử dụng
    printf("DEBUG: len = %d\n", in_msg->len);
    in_msg->value[in_msg->len] = '\0'; 
    const char* received_value = in_msg->value;

    switch (in_msg->type) {
        case MSG_LOGIN: {
            printf("[SERVER]|[THREAD %lu] Nhận yêu cầu LOGIN: %s\n", (unsigned long)thread_id, received_value);

            // 1. Kiểm tra Login Name (Giả định mọi tên không rỗng đều hợp lệ)
            if (in_msg->len > 0) {


                if(client_list != NULL && strcmp(searchUsernameList(client_list, in_msg->value).username, "") != 0){
                    // Tai khoan ton tai

                    // 2. Server đồng ý kết nối và ghi nhớ tên [1, 2] - Lưu vào thread-local variable
                    strncpy(logged_in_username, received_value, in_msg->len);
                    logged_in_username[in_msg->len] = '\0'; // Ensure null-terminated
                
                    // 3. Phản hồi bằng MSG_CF [2]
                    // Can xu ly kiem tra ton tai tai khoan USER
                    create_message(out_msg, MSG_CF, logged_in_username);
                    printf("[SERVER]|[THREAD %lu] Gửi phản hồi: CONFIRM cho %s \n", (unsigned long)thread_id, logged_in_username);

                }else if(client_list != NULL){
                    // Tai khoan khong ton tai
                    create_message(out_msg, MSG_DENY, "Login name does not exist.");
                    printf("[SERVER]|[THREAD %lu] Gửi phản hồi: DENY (tài khoản không tồn tại) \n", (unsigned long)thread_id);
                }
            } else {
                // 4. Nếu tên rỗng, phản hồi DENY
                create_message(out_msg, MSG_DENY, "Login name cannot be empty.");
                printf("[SERVER]|[THREAD %lu] Gửi phản hồi: DENY (tên rỗng) \n", (unsigned long)thread_id);
            }
            break;
        }

        case MSG_TEXT: {
            // Chỉ xử lý tin nhắn TEXT nếu Client đã đăng nhập [2]
            if (strlen(logged_in_username) > 0) {
                printf("[SERVER]|[THREAD %lu] Nhận TEXT từ [%s] : %s\n", (unsigned long)thread_id, logged_in_username, received_value);
                
                // 1. Lưu dữ liệu Text message vào log riêng của Client [1, 2]
                server_log_msg(logged_in_username, in_msg, thread_id);

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
        case MSG_DENY: {
            printf("[CLIENT] Server DENY (0x%02X): ERROR: %s\n", in_msg->type, recv_value);
            // Client nhận Deny message -> Thực hiện lại gửi login message cho Server [2]
            break;
            
        }
        default: {
            printf("[CLIENT] Phản hồi không mong muốn (0x%02X).\n", in_msg->type);
            break;

        }
    }
}
