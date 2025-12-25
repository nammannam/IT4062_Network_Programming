/*
Nguyen Khanh Nam - 20225749
W6 - Protocol Design

Mô tả giao thức đơn giản giữa Client và Server với các loại thông điệp:
1. MSG_LOGIN (0x01): Client gửi tên đăng nhập đến Server.
2. MSG_CF (0x11): Server xác nhận đăng nhập thành công.
3. MSG_TEXT (0x10): Client gửi tin nhắn văn bản đến Server.
4. MSG_DENY (0x00): Server từ chối yêu cầu của Client.


*/


#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#include "llist.h"

#define MAX_VALUE_LEN 256
#define HEADER_SIZE (sizeof(uint8_t) * 2)
#define MAX_CLIENTS 100
#define SHM_KEY 0x1234

// Shared memory structure to track logged-in clients
typedef struct {
    char username[MAX_LEN];
    pid_t pid;
    int active;  // 1 = active, 0 = inactive
} LoggedInClient_s;

typedef struct {
    LoggedInClient_s clients[MAX_CLIENTS];
    int count;
} SharedClientList_s;

// Message Types
typedef enum
{
    MSG_DENY = 0x00,
    MSG_CF = 0x11,
    MSG_LOGIN = 0x01,
    MSG_PASSWORD = 0x03,
    MSG_TEXT = 0x10,
    MSG_WHO = 0x02,
    MSG_LIST = 0x12,
    MSG_HELP = 0x04

} msg_type_t;

// Message = Header + Body
// Sử dụng packing 1 byte để loại bỏ padding, đảm bảo struct ánh xạ chính xác lên mạng
#pragma pack(push, 1)
typedef struct
{
    uint8_t type;
    uint8_t len;
    char value[MAX_VALUE_LEN];

} application_msg_t;
#pragma pack(pop)

// Util functions
void create_message(application_msg_t *msg, msg_type_t type, const char *data);

void server_handle_message(application_msg_t *in_msg, application_msg_t *out_msg, Llist_s *client_list, pid_t pid_client, const char *client_ip, int client_port, char *pending_username, char *logged_in_username);

void client_handle_response(application_msg_t *msg);

void server_log_msg(const char *username, application_msg_t *msg, pid_t pid_client);

// Shared memory functions
SharedClientList_s* init_shared_memory();
void add_logged_in_client(SharedClientList_s *shm, const char *username, pid_t pid);
void remove_logged_in_client(SharedClientList_s *shm, pid_t pid);
void get_logged_in_clients_list(SharedClientList_s *shm, char *output_buffer, size_t buffer_size);

// Authentication logging functions
void log_login_success(const char *username, const char *client_ip, int client_port);
void log_login_fail(const char *username, const char *client_ip, int client_port, const char *reason);
void log_account_locked(const char *username);
void log_logout(const char *username, const char *client_ip, int client_port);

// Account management functions
void lock_account_in_file(const char *username, const char *filepath);
void lock_account_in_list(Llist_s *list, const char *username);



#endif // PROTOCOL_H