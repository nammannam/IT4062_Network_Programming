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
#include <pthread.h>

#include "llist.h"

#define MAX_VALUE_LEN 256
#define HEADER_SIZE (sizeof(uint8_t) * 2)

// Message Types
typedef enum
{
    MSG_DENY = 0x00,
    MSG_CF = 0x11,
    MSG_LOGIN = 0x01,
    MSG_TEXT = 0x10

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

void server_handle_message(application_msg_t *in_msg, application_msg_t *out_msg, Llist_s *client_list, pthread_t thread_id, char *logged_in_username);

void client_handle_response(application_msg_t *msg);

void server_log_msg(const char *username, application_msg_t *msg, pthread_t thread_id);



#endif // PROTOCOL_H