/*
================================================================================
Nguyen Khanh Nam - 20225749
W8 - Multi-threading TCP Server with pthread

MÔ TẢ GIAO THỨC:
----------------
Giao thức client-server hỗ trợ đăng nhập với username/password,
gửi text message, và các lệnh quản lý (who, help).

CÁC LOẠI MESSAGE:
-----------------
1. MSG_LOGIN (0x01):    Client gửi username đến Server
2. MSG_PASSWORD (0x03): Client gửi password / Server yêu cầu password
3. MSG_CF (0x11):       Server xác nhận thành công
4. MSG_DENY (0x00):     Server từ chối yêu cầu
5. MSG_TEXT (0x10):     Client gửi tin nhắn văn bản (sau khi đăng nhập)
6. MSG_WHO (0x02):      Client yêu cầu danh sách users đang online
7. MSG_LIST (0x12):     Server trả về danh sách users
8. MSG_HELP (0x04):     Client yêu cầu danh sách lệnh

KIẾN TRÚC MULTI-THREADING:
--------------------------
- Server sử dụng pthread_create() để tạo một thread riêng cho mỗi client
- SharedClientList_s với pthread_mutex_t đảm bảo thread-safe access
- Mỗi thread có logged_in_username và pending_username riêng (thread-local)
  để tránh race condition

BẢO MẬT:
--------
- Tài khoản bị khóa tự động sau 3 lần nhập sai password
- Tất cả hoạt động đăng nhập được ghi log vào ./log/auth.log
- Text messages được lưu vào ./log/user.txt

================================================================================
*/


#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <pthread.h>

#include "llist.h"

#define MAX_VALUE_LEN 256
#define HEADER_SIZE (sizeof(uint8_t) * 2)
#define MAX_CLIENTS 100

// ============================================================================
// SHARED CLIENT LIST STRUCTURES (THREAD-SAFE)
// ============================================================================

/**
 * @brief Cấu trúc lưu thông tin một client đã đăng nhập
 * @note Được sử dụng trong SharedClientList_s để track các client đang online
 */
typedef struct {
    char username[MAX_LEN];     // Username của client
    pthread_t thread_id;        // Thread ID đang xử lý client này
    int active;                 // 1 = active (đang đăng nhập), 0 = inactive
} LoggedInClient_s;

/**
 * @brief Danh sách thread-safe các client đã đăng nhập
 * @note Sử dụng mutex để đảm bảo thread-safe khi nhiều threads đồng thời truy cập
 * @note Thay thế System V shared memory trong kiến trúc multi-threading
 */
typedef struct {
    LoggedInClient_s clients[MAX_CLIENTS];  // Mảng các client đã đăng nhập
    int count;                              // Số lượng client đang online
    pthread_mutex_t mutex;                  // Mutex để bảo vệ truy cập đồng thời
} SharedClientList_s;

// ============================================================================
// MESSAGE PROTOCOL DEFINITIONS
// ============================================================================

/**
 * @brief Các loại message trong giao thức client-server
 * 
 * Protocol flow:
 * 1. Client -> Server: MSG_LOGIN (username)
 * 2. Server -> Client: MSG_PASSWORD (nếu tài khoản hợp lệ) hoặc MSG_DENY (nếu lỗi)
 * 3. Client -> Server: MSG_PASSWORD (password)
 * 4. Server -> Client: MSG_CF (thành công) hoặc MSG_DENY (thất bại)
 * 5. Client -> Server: MSG_TEXT / MSG_WHO / MSG_HELP (sau khi đăng nhập)
 * 6. Server -> Client: MSG_CF / MSG_LIST / MSG_DENY (tùy request)
 */
typedef enum
{
    MSG_DENY = 0x00,        // Server từ chối request (lỗi hoặc chưa đăng nhập)
    MSG_CF = 0x11,          // Server xác nhận thành công
    MSG_LOGIN = 0x01,       // Client gửi username
    MSG_PASSWORD = 0x03,    // Client gửi password / Server yêu cầu password
    MSG_TEXT = 0x10,        // Client gửi text message (sau khi đăng nhập)
    MSG_WHO = 0x02,         // Client yêu cầu danh sách users đang online
    MSG_LIST = 0x12,        // Server gửi danh sách users
    MSG_HELP = 0x04         // Client yêu cầu danh sách lệnh

} msg_type_t;

/**
 * @brief Cấu trúc message trong giao thức application layer
 * @note Sử dụng #pragma pack(1) để loại bỏ padding, đảm bảo struct
 *       ánh xạ chính xác trên network (byte alignment)
 * 
 * Format:
 * - Header: 2 bytes (type + len)
 * - Body: tối đa MAX_VALUE_LEN bytes (value)
 * 
 * Example:
 * type=0x01, len=5, value="admin" -> [0x01][0x05][a][d][m][i][n]
 */
// Message = Header + Body
// Sử dụng packing 1 byte để loại bỏ padding, đảm bảo struct ánh xạ chính xác lên mạng
#pragma pack(push, 1)
typedef struct
{
    uint8_t type;               // Loại message (msg_type_t)
    uint8_t len;                // Độ dài của value (0-255)
    char value[MAX_VALUE_LEN];  // Nội dung message (chuỗi kết thúc bằng '\0')

} application_msg_t;
#pragma pack(pop)

// ============================================================================
// MESSAGE CREATION AND HANDLING FUNCTIONS
// ============================================================================

/**
 * @brief Tạo một application message với type và data cho trước
 * @param msg Pointer đến message cần tạo
 * @param type Loại message (MSG_LOGIN, MSG_CF, MSG_TEXT, MSG_DENY, etc.)
 * @param data Dữ liệu chuỗi cần gửi (có thể NULL)
 * @note Hàm sẽ tự động cắt data nếu vượt quá MAX_VALUE_LEN
 */
void create_message(application_msg_t *msg, msg_type_t type, const char *data);

/**
 * @brief Xử lý message từ client tại server side
 * @param in_msg Message nhận được từ client
 * @param out_msg Message phản hồi gửi về client
 * @param client_list Danh sách tài khoản từ file account.txt
 * @param thread_id ID của thread đang xử lý client này
 * @param client_ip Địa chỉ IP của client
 * @param client_port Port của client
 * @param shared_clients Danh sách các client đã đăng nhập (thread-safe)
 * @param logged_in_username Buffer lưu username đã đăng nhập (thread-local)
 * @param pending_username Buffer lưu username đang chờ nhập password (thread-local)
 * @note Hàm xử lý các loại message: MSG_LOGIN, MSG_PASSWORD, MSG_TEXT, MSG_WHO, MSG_HELP
 * @note logged_in_username và pending_username PHẢI là thread-local để tránh race condition
 */
void server_handle_message(application_msg_t *in_msg, application_msg_t *out_msg, Llist_s *client_list, pthread_t thread_id, const char *client_ip, int client_port, SharedClientList_s *shared_clients, char *logged_in_username, char *pending_username);

/**
 * @brief Xử lý response từ server tại client side
 * @param msg Message nhận được từ server
 * @note Hàm in ra màn hình nội dung phản hồi (MSG_CF, MSG_DENY, MSG_PASSWORD, MSG_LIST)
 */
void client_handle_response(application_msg_t *msg);

/**
 * @brief Ghi log message từ client vào file log của user
 * @param username Username của client gửi message
 * @param msg Message cần ghi log
 * @param thread_id ID của thread đang xử lý
 * @note Log được ghi vào ./log/user.txt
 */
void server_log_msg(const char *username, application_msg_t *msg, pthread_t thread_id);

// ============================================================================
// THREAD-SAFE CLIENT LIST MANAGEMENT FUNCTIONS
// ============================================================================

/**
 * @brief Khởi tạo shared client list với mutex
 * @param list Pointer đến SharedClientList_s cần khởi tạo
 * @note Hàm khởi tạo mutex để đảm bảo thread-safe khi nhiều threads truy cập
 */
void init_client_list(SharedClientList_s *list);

/**
 * @brief Thêm một client đã đăng nhập vào shared list (thread-safe)
 * @param list Pointer đến shared client list
 * @param username Username của client
 * @param thread_id Thread ID đang xử lý client này
 * @note Hàm sử dụng mutex lock để đảm bảo thread-safe
 */
void add_logged_in_client(SharedClientList_s *list, const char *username, pthread_t thread_id);

/**
 * @brief Xóa một client khỏi shared list khi logout (thread-safe)
 * @param list Pointer đến shared client list
 * @param thread_id Thread ID của client cần xóa
 * @note Hàm sử dụng mutex lock để đảm bảo thread-safe
 */
void remove_logged_in_client(SharedClientList_s *list, pthread_t thread_id);

/**
 * @brief Lấy danh sách các client đang đăng nhập dạng chuỗi (thread-safe)
 * @param list Pointer đến shared client list
 * @param output_buffer Buffer để lưu chuỗi kết quả
 * @param buffer_size Kích thước của output buffer
 * @note Format: "user1, user2, user3" hoặc "No users currently logged in."
 * @note Hàm sử dụng mutex lock để đảm bảo thread-safe
 */
void get_logged_in_clients_list(SharedClientList_s *list, char *output_buffer, size_t buffer_size);

// ============================================================================
// AUTHENTICATION LOGGING FUNCTIONS
// ============================================================================

/**
 * @brief Ghi log khi đăng nhập thành công
 * @param username Username đã đăng nhập
 * @param client_ip Địa chỉ IP của client
 * @param client_port Port của client
 * @note Log được ghi vào ./log/auth.log với timestamp
 */
void log_login_success(const char *username, const char *client_ip, int client_port);

/**
 * @brief Ghi log khi đăng nhập thất bại
 * @param username Username cố gắng đăng nhập
 * @param client_ip Địa chỉ IP của client
 * @param client_port Port của client
 * @param reason Lý do thất bại (wrong password, account blocked, etc.)
 * @note Log được ghi vào ./log/auth.log với timestamp
 */
void log_login_fail(const char *username, const char *client_ip, int client_port, const char *reason);

/**
 * @brief Ghi log khi tài khoản bị khóa
 * @param username Username bị khóa
 * @note Log được ghi vào ./log/auth.log với timestamp
 */
void log_account_locked(const char *username);

/**
 * @brief Ghi log khi user logout
 * @param username Username đăng xuất
 * @param client_ip Địa chỉ IP của client
 * @param client_port Port của client
 * @note Log được ghi vào ./log/auth.log với timestamp
 */
void log_logout(const char *username, const char *client_ip, int client_port);

// ============================================================================
// ACCOUNT MANAGEMENT FUNCTIONS
// ============================================================================

/**
 * @brief Khóa tài khoản trong file account.txt (đổi status từ '1' thành '0')
 * @param username Username cần khóa
 * @param filepath Đường dẫn đến file account.txt
 * @note Hàm đọc toàn bộ file, update status, và ghi lại file
 */
void lock_account_in_file(const char *username, const char *filepath);

/**
 * @brief Khóa tài khoản trong in-memory linked list (đổi status từ '1' thành '0')
 * @param list Pointer đến linked list chứa thông tin tài khoản
 * @param username Username cần khóa
 * @note Cập nhật trong bộ nhớ để không cần reload file
 */
void lock_account_in_list(Llist_s *list, const char *username);



#endif // PROTOCOL_H