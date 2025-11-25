# Race Condition Fix - Thread Safety trong Server pthread

## Vấn đề gặp phải

### Hiện tượng
Khi nhiều clients kết nối đồng thời:
- **Log file bị ghi nhầm tên user**: Thread A đăng nhập user "admin", Thread B đăng nhập user "namnk", nhưng log của admin lại hiện tên "namnk"
- **Server xử lý protocol bị nhầm lẫn**: Client A gửi message nhưng server nghĩ là từ Client B
- **In ra terminal bị trùng tên**: Printf hiển thị sai username cho từng thread

### Root Cause - Global Variables

#### 1. server.c (ĐÃ SỬA)
```c
// BEFORE - GLOBAL VARIABLES (RACE CONDITION!)
AccountInfo_s loggedInUser; // ❌ TẤT CẢ threads chia sẻ chung
int isLoggedIn = 0;         // ❌ TẤT CẢ threads chia sẻ chung
```

**Vấn đề:** 
- Thread 1: `loggedInUser = "admin"` 
- Thread 2: `loggedInUser = "namnk"` → GHI ĐÈ lên Thread 1!
- Thread 1 đọc lại → Nhận được `"namnk"` thay vì `"admin"`

#### 2. protocol_utils.c (ĐÃ SỬA)
```c
// BEFORE - GLOBAL VARIABLE (RACE CONDITION!)
char logged_in_client[64]; // ❌ TẤT CẢ threads chia sẻ chung

void server_handle_message(...) {
    strncpy(logged_in_client, username, ...); // ❌ Race condition!
    server_log_msg(logged_in_client, ...);    // ❌ Có thể log sai user
}
```

**Vấn đề:**
- Thread 1 ghi: `logged_in_client = "admin"`
- Thread 2 ghi: `logged_in_client = "namnk"` → GHI ĐÈ!
- Thread 1 log: Ghi ra "namnk" thay vì "admin"

## Giải pháp - Thread-Local Storage

### 1. Sửa server.c - Thread Session

```c
// AFTER - THREAD-LOCAL STORAGE (THREAD-SAFE!)

// Định nghĩa struct cho session của mỗi thread
typedef struct {
    AccountInfo_s loggedInUser; // ✅ Mỗi thread có biến riêng
    int isLoggedIn;             // ✅ Mỗi thread có biến riêng
} thread_session_t;

void* handle_client(void* arg) {
    // ✅ Tạo session LOCAL cho thread này
    thread_session_t session;
    session.isLoggedIn = 0;
    memset(&session.loggedInUser, 0, sizeof(session.loggedInUser));
    
    // ✅ Thread-local username cho protocol
    char logged_in_username[64];
    memset(logged_in_username, 0, sizeof(logged_in_username));
    
    // Mỗi thread có bản copy riêng của session và logged_in_username
    // Thread 1: session.loggedInUser = "admin"
    // Thread 2: session.loggedInUser = "namnk"
    // KHÔNG GHI ĐÈ lên nhau!
}
```

### 2. Sửa protocol_utils.c - Thread-Local Parameter

```c
// AFTER - PASS AS PARAMETER (THREAD-SAFE!)

// ✅ KHÔNG có global variable nữa

void server_handle_message(
    application_msg_t *in_msg, 
    application_msg_t *out_msg, 
    Llist_s *client_list, 
    pthread_t thread_id,
    char *logged_in_username  // ✅ Nhận từ thread-local variable
) {
    // MSG_LOGIN
    strncpy(logged_in_username, received_value, in_msg->len);
    logged_in_username[in_msg->len] = '\0';
    
    // MSG_TEXT
    server_log_msg(logged_in_username, in_msg, thread_id);
    // ✅ Mỗi thread truyền username riêng của nó
}
```

### 3. Sửa protocol.h - Update Function Signature

```c
// BEFORE
void server_handle_message(
    application_msg_t *in_msg, 
    application_msg_t *out_msg, 
    Llist_s *client_list, 
    pthread_t thread_id
);

// AFTER
void server_handle_message(
    application_msg_t *in_msg, 
    application_msg_t *out_msg, 
    Llist_s *client_list, 
    pthread_t thread_id,
    char *logged_in_username  // ✅ Thêm parameter
);
```

### 4. Update Function Calls

```c
// BEFORE
accountSignIn(list, INPUT_FILE_PATH, username, password, &attempts);
accountSignOut();
changePassword(list, newPassword, reply);

// AFTER - Truyền session pointer
accountSignIn(list, INPUT_FILE_PATH, username, password, &attempts, &session);
accountSignOut(&session);
changePassword(list, newPassword, reply, &session);

// BEFORE
server_handle_message(&msgIn, &msgOut, list, thread_id);

// AFTER - Truyền logged_in_username
server_handle_message(&msgIn, &msgOut, list, thread_id, logged_in_username);
```

## Thread Safety với Mutex

### File I/O Protection

```c
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

// Protect file writes
pthread_mutex_lock(&file_mutex);
AccountInfo_s accountInfo = searchAccountInFile(INPUT_FILE_PATH, bufferUsername);
pthread_mutex_unlock(&file_mutex);

pthread_mutex_lock(&file_mutex);
updateAccountStatusInFile(filePath, username, '0');
updateAccountListFromFile(filePath, list);
pthread_mutex_unlock(&file_mutex);

pthread_mutex_lock(&file_mutex);
updateAccountToFile(INPUT_FILE_PATH, list);
pthread_mutex_unlock(&file_mutex);
```

### Account List Protection

```c
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;

// Protect account list access (if needed)
pthread_mutex_lock(&list_mutex);
AccountInfo_s acc = searchUsernameList(list, username);
pthread_mutex_unlock(&list_mutex);
```

## Kết quả

### Before Fix (Race Condition)
```
Thread 1: Client admin đăng nhập
Thread 2: Client namnk đăng nhập
Thread 1: Log file ghi "namnk" thay vì "admin" ❌
Thread 1: Server nghĩ client là "namnk" ❌
Thread 2: Server nghĩ client là "namnk" ✓ (may mắn đúng)
```

### After Fix (Thread-Safe)
```
Thread 1: Client admin đăng nhập
Thread 2: Client namnk đăng nhập
Thread 1: Log file ghi "admin" ✓
Thread 1: Server biết client là "admin" ✓
Thread 2: Log file ghi "namnk" ✓
Thread 2: Server biết client là "namnk" ✓
```

## Memory Layout Visualization

### Before (Global Variables)
```
┌─────────────────────────────────────┐
│         GLOBAL MEMORY               │
│  loggedInUser = "???"               │ ← TẤT CẢ threads ghi vào đây
│  isLoggedIn = ?                     │ ← RACE CONDITION!
│  logged_in_client = "???"           │ ← GHI ĐÈ lên nhau
└─────────────────────────────────────┘
      ↑           ↑           ↑
   Thread 1   Thread 2   Thread 3
```

### After (Thread-Local Storage)
```
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│  Thread 1    │  │  Thread 2    │  │  Thread 3    │
│  Stack       │  │  Stack       │  │  Stack       │
├──────────────┤  ├──────────────┤  ├──────────────┤
│ session:     │  │ session:     │  │ session:     │
│  .loggedIn   │  │  .loggedIn   │  │  .loggedIn   │
│   User="ad.."│  │   User="na.."│  │   User="us.."│
│  .isLogged=1 │  │  .isLogged=1 │  │  .isLogged=1 │
│              │  │              │  │              │
│ logged_in_   │  │ logged_in_   │  │ logged_in_   │
│  username:   │  │  username:   │  │  username:   │
│  "admin"     │  │  "namnk"     │  │  "user3"     │
└──────────────┘  └──────────────┘  └──────────────┘
    ✅ ISOLATED     ✅ ISOLATED     ✅ ISOLATED
```

## Best Practices Applied

1. ✅ **Avoid Global Variables in Multi-threaded Code**
   - Chuyển sang thread-local storage (stack variables)
   - Pass as function parameters instead

2. ✅ **Use Mutex for Shared Resources**
   - File I/O: `file_mutex`
   - Account list: `list_mutex`

3. ✅ **Thread ID for Logging**
   - `pthread_self()` để track từng thread
   - Log format: `THREAD 139876543210: | Client: admin | ...`

4. ✅ **Clean Memory Management**
   - `malloc()` trong main, `free()` trong thread
   - Thread-local variables tự động cleanup khi thread exit

## Testing

### Test với nhiều clients đồng thời:
```bash
# Terminal 1
./server 5500

# Terminal 2-5 (chạy đồng thời)
./client 127.0.0.1 5500  # Login: admin
./client 127.0.0.1 5500  # Login: namnk
./client 127.0.0.1 5500  # Login: user1
./client 127.0.0.1 5500  # Login: user2
```

### Kiểm tra log file:
```bash
cat log/user.txt
```

**Expected Output (Thread-Safe):**
```
THREAD 139876543210: | Client: admin | Type=0x01 | Length=5 | Value="admin"
THREAD 139876543211: | Client: namnk | Type=0x01 | Length=5 | Value="namnk"
THREAD 139876543212: | Client: user1 | Type=0x01 | Length=5 | Value="user1"
THREAD 139876543210: | Client: admin | Type=0x10 | Length=11 | Value="hello world"
THREAD 139876543211: | Client: namnk | Type=0x10 | Length=8 | Value="test msg"
```

✅ Mỗi thread ghi đúng username của client của nó!

## Performance Impact

- **Memory overhead**: Minimal (mỗi thread thêm ~100 bytes cho session + logged_in_username)
- **CPU overhead**: None (không có lock contention cho session variables)
- **Mutex overhead**: Chỉ khi access file/shared list (cần thiết)

## Tổng kết

| Trước | Sau |
|---|---|
| ❌ Global variables | ✅ Thread-local storage |
| ❌ Race conditions | ✅ Thread-safe |
| ❌ Log nhầm user | ✅ Log đúng user |
| ❌ Protocol nhầm client | ✅ Protocol đúng client |
| ⚠️ Không có mutex | ✅ Mutex cho file I/O |

**Kết luận**: Code hiện tại đã thread-safe cho session management và protocol handling! 🎉
