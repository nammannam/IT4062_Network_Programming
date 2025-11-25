# Multi-threading TCP Server with pthread

**Nguyen Khanh Nam - 20225749**  
**IT4062 - Network Programming - Week 8**

---

## 📋 Mô tả Project

Hệ thống client-server sử dụng giao thức TCP với kiến trúc **multi-threading** (pthread). Server có khả năng xử lý nhiều client đồng thời, mỗi client được xử lý bởi một thread riêng biệt.

### Tính năng chính:
- ✅ **Authentication System**: Đăng nhập với username/password
- ✅ **Account Security**: Tự động khóa tài khoản sau 3 lần đăng nhập sai
- ✅ **Multi-threading**: Xử lý nhiều clients đồng thời với pthread
- ✅ **Thread-safe**: Sử dụng mutex để bảo vệ shared resources
- ✅ **Logging System**: Ghi log tất cả hoạt động authentication và messages
- ✅ **Online Users**: Hiển thị danh sách users đang online (lệnh `who`)
- ✅ **Help Command**: Hướng dẫn sử dụng các lệnh (lệnh `help`)

---

## 🏗️ Kiến trúc Hệ thống

### Multi-threading Architecture

```
                    ┌─────────────────┐
                    │   TCP Server    │
                    │   (Port 8080)   │
                    └────────┬────────┘
                             │
                ┌────────────┴────────────┐
                │   accept() - Main Loop  │
                └────────────┬────────────┘
                             │
        ┌────────────────────┼────────────────────┐
        │                    │                    │
   ┌────▼────┐         ┌────▼────┐         ┌────▼────┐
   │ Thread 1│         │ Thread 2│   ...   │ Thread N│
   │Client A │         │Client B │         │Client N │
   └─────────┘         └─────────┘         └─────────┘
        │                    │                    │
        │                    │                    │
   ┌────▼────────────────────▼────────────────────▼────┐
   │      SharedClientList_s (with pthread_mutex)      │
   │  Thread-safe list of logged-in clients            │
   └───────────────────────────────────────────────────┘
```

### Thread Safety

- **Global Structure**: `SharedClientList_s` với `pthread_mutex_t`
- **Thread-local Variables**: Mỗi thread có `logged_in_username` và `pending_username` riêng
- **Race Condition Fix**: Không sử dụng global variables cho username/password

---

## 📡 Giao thức Application Layer

### Message Format

```
┌──────────┬──────────┬─────────────────────────┐
│   type   │   len    │         value           │
│ (1 byte) │ (1 byte) │  (max 256 bytes)        │
└──────────┴──────────┴─────────────────────────┘
```

### Message Types

| Code | Type         | Direction | Mô tả                                |
|------|--------------|-----------|--------------------------------------|
| 0x01 | MSG_LOGIN    | C → S     | Client gửi username                  |
| 0x03 | MSG_PASSWORD | C ⇄ S     | Client gửi password / Server yêu cầu |
| 0x11 | MSG_CF       | S → C     | Server xác nhận thành công           |
| 0x00 | MSG_DENY     | S → C     | Server từ chối (lỗi)                 |
| 0x10 | MSG_TEXT     | C → S     | Client gửi text message              |
| 0x02 | MSG_WHO      | C → S     | Client yêu cầu danh sách users       |
| 0x12 | MSG_LIST     | S → C     | Server trả về danh sách users        |
| 0x04 | MSG_HELP     | C → S     | Client yêu cầu help                  |

### Protocol Flow

```
Client                                  Server
  │                                        │
  ├──────── MSG_LOGIN (username) ────────>│
  │                                        ├─ Check account exists
  │<──────── MSG_PASSWORD ─────────────────┤   and status
  │          (or MSG_DENY if error)        │
  │                                        │
  ├──────── MSG_PASSWORD (password) ──────>│
  │                                        ├─ Verify password
  │<──────── MSG_CF (success) ─────────────┤   Track failed attempts
  │          (or MSG_DENY if fail)         │   Lock after 3 fails
  │                                        │
  ├──────── MSG_TEXT / WHO / HELP ────────>│
  │                                        │
  │<──────── MSG_CF / LIST ────────────────┤
  │                                        │
```

---

## 🔐 Bảo mật

### Account Locking Mechanism

1. **Failed Login Tracking**: Server theo dõi số lần đăng nhập sai cho mỗi username
2. **Auto-lock After 3 Attempts**: Tài khoản tự động bị khóa (status = '0')
3. **Persistent Lock**: Lock được ghi vào cả file `account.txt` và in-memory list
4. **Reset on Success**: Counter được reset về 0 khi đăng nhập thành công

### Authentication Logging

Tất cả hoạt động authentication được ghi vào `./log/auth.log`:

```
[2025-11-25 10:30:15] LOGIN admin from 127.0.0.1:54321 SUCCESS
[2025-11-25 10:31:22] LOGIN namnk from 127.0.0.1:54322 FAIL (wrong password)
[2025-11-25 10:31:30] ACCOUNT_LOCKED namnk
[2025-11-25 10:35:45] LOGOUT admin from 127.0.0.1:54321
```

---

## 📁 Cấu trúc Project

```
W8/
├── server.c              # TCP server với pthread
├── client.c              # TCP client
├── protocol.h            # Protocol definitions và function declarations
├── protocol_utils.c      # Implementation của protocol handling
├── llist.h               # Linked list header (account management)
├── llist_utils.c         # Linked list implementation
├── Makefile             # Build configuration
├── account.txt          # Database của accounts (username password email homepage status)
├── README.md            # Documentation (file này)
├── log/
│   ├── auth.log         # Authentication logs
│   └── user.txt         # User messages logs
└── onclass/             # Reference implementations
    └── RACE_CONDITION_FIX.md
```

---

## 🛠️ Compile và Chạy

### Requirements
- GCC compiler
- POSIX threads (pthread)
- Linux/Unix environment

### Build

```bash
make clean
make
```

Hoặc compile thủ công:

```bash
gcc -Wall -Wextra -g -pthread -c server.c -o server.o
gcc -Wall -Wextra -g -pthread -c client.c -o client.o
gcc -Wall -Wextra -g -pthread -c protocol_utils.c -o protocol_utils.o
gcc -Wall -Wextra -g -pthread -c llist_utils.c -o llist_utils.o

gcc -pthread -o server server.o protocol_utils.o llist_utils.o
gcc -pthread -o client client.o protocol_utils.o llist_utils.o
```

### Run Server

```bash
./server <port>
```

Example:
```bash
./server 8080
```

### Run Client

```bash
./client <server_ip> <server_port>
```

Example:
```bash
./client 127.0.0.1 8080
```

---

## 🎮 Hướng dẫn Sử dụng

### 1. Đăng nhập

```
Client: admin
Server: Enter password:
Client: admin123
Server: Login successful
```

### 2. Gửi text message

```
Client: Hello, this is a test message
Server: CONFIRM (message saved to log)
```

### 3. Xem danh sách users online

```
Client: who
Server: Logged-in users: admin, namnk, locdinh
```

### 4. Xem help

```
Client: help
Server: 
=== AVAILABLE COMMANDS ===
who  - Display list of logged-in users
help - Display this help message
bye  - Logout and disconnect
==========================
```

### 5. Logout

```
Client: bye
Server: Goodbye!
(Connection closed)
```

---

## 📊 Database Format

### account.txt

Format: `username password email homepage status`

```
admin admin123 admin@gmail.com google.com 1
namnk namnk123 namnk@gmail.com youtube.com 1
locdinh loc2004 123@gmail.com daotao.ai 1
```

**Status:**
- `1` = Active account
- `0` = Blocked account

---

## 🐛 Debugging và Testing

### Test Multi-threading

Mở nhiều terminals và chạy clients đồng thời:

```bash
# Terminal 1
./client 127.0.0.1 8080

# Terminal 2
./client 127.0.0.1 8080

# Terminal 3
./client 127.0.0.1 8080
```

### Test Account Locking

1. Client 1: Đăng nhập sai 3 lần với username `test2`
2. Kiểm tra `account.txt` - status của `test2` đã đổi thành `0`
3. Kiểm tra `./log/auth.log` - có log `ACCOUNT_LOCKED test2`
4. Client 2: Thử đăng nhập với `test2` - bị từ chối ngay

### Check Logs

```bash
# Authentication logs
tail -f ./log/auth.log

# User messages
cat ./log/user.txt
```

### Debug với gdb

```bash
gcc -g -pthread server.c protocol_utils.c llist_utils.c -o server
gdb ./server
```

---

## 🔧 Key Implementation Details

### 1. Thread Creation

```c
pthread_t thread_id;
thread_arg_t *thread_arg = malloc(sizeof(thread_arg_t));
thread_arg->connfd = connfd;
thread_arg->cliaddr = cliaddr;
thread_arg->account_list = list;

pthread_create(&thread_id, NULL, handle_client_thread, thread_arg);
pthread_detach(thread_id);  // Auto cleanup when thread exits
```

### 2. Mutex Protection

```c
pthread_mutex_lock(&shared_clients->mutex);
// Critical section - modify shared_clients
pthread_mutex_unlock(&shared_clients->mutex);
```

### 3. Thread-local Storage

```c
void *handle_client_thread(void *arg) {
    // Each thread has its own copies (on stack)
    char logged_in_username[MAX_LEN];
    char pending_username[MAX_LEN];
    
    // Pass to handler function
    server_handle_message(..., logged_in_username, pending_username);
}
```

---

## 📈 Improvements từ Week 6 (fork-based)

| Aspect            | Week 6 (fork)          | Week 8 (pthread)       |
|-------------------|------------------------|------------------------|
| Process model     | Multi-process          | Multi-threading        |
| IPC mechanism     | System V Shared Memory | Mutex + shared struct  |
| Memory usage      | High (copy-on-write)   | Low (shared address)   |
| Context switch    | Slower                 | Faster                 |
| Resource cleanup  | waitpid()              | pthread_detach()       |
| Race condition    | None (separate memory) | Fixed with thread-local|

---

## 🚨 Known Issues & Solutions

### Issue 1: Race Condition with Global Variables

**Problem:** Multiple threads share global `logged_in_client` và `pending_username`

```c
// ❌ WRONG - Race condition
char logged_in_client[64];  // Global variable
char pending_username[64];  // Global variable
```

**Solution:** Thread-local storage

```c
// ✅ CORRECT - Each thread has its own copy
void *handle_client_thread(void *arg) {
    char logged_in_username[MAX_LEN];   // Stack variable
    char pending_username[MAX_LEN];     // Stack variable
}
```

### Issue 2: Sizeof pointer vs array

**Problem:** `sizeof(pending_username)` khi parameter là pointer

```c
memset(pending_username, 0, sizeof(pending_username));  // Wrong!
```

**Solution:** Use constant

```c
memset(pending_username, 0, MAX_LEN);  // Correct
```

---

## 📚 References

- POSIX Threads Programming: https://hpc-tutorials.llnl.gov/posix/
- TCP Socket Programming: Beej's Guide to Network Programming
- Thread Synchronization: pthread_mutex documentation

---

## 👨‍💻 Author

**Nguyen Khanh Nam**  
Student ID: 20225749  
Course: IT4062 - Network Programming  
Semester: 2025.1  

---

## 📝 License

This project is for educational purposes only.

---

## 🎯 Future Enhancements

- [ ] Implement private messaging between users
- [ ] Add SSL/TLS encryption
- [ ] Support for file transfer
- [ ] Web-based admin panel
- [ ] Database integration (SQLite/MySQL)
- [ ] Connection pooling
- [ ] Rate limiting per user
