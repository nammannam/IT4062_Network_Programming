# Kiểm tra Status Đăng Nhập

## Tóm tắt
Đã sửa lại cơ chế đăng nhập để kiểm tra **status** tài khoản trong `account.txt`:
- **status = '1'**: Tài khoản ACTIVE → Cho phép đăng nhập
- **status = '0'**: Tài khoản BLOCKED → Từ chối đăng nhập

## Thay đổi trong `protocol_utils.c`

### Case MSG_PASSWORD (dòng 205-245):
```c
case MSG_PASSWORD: {
    if (strlen(pending_username) > 0) {
        // Kiểm tra password
        AccountInfo_s acc = searchUsernameList(client_list, pending_username);
        
        if (strcmp(acc.password, received_value) == 0) {
            // Password đúng - Kiểm tra status
            if (acc.status == '1') {
                // ✅ Tài khoản ACTIVE - Đăng nhập thành công
                strncpy(logged_in_client, pending_username, sizeof(logged_in_client) - 1);
                
                // Thêm vào shared memory
                SharedClientList_s *shm = init_shared_memory();
                if (shm != NULL) {
                    add_logged_in_client(shm, logged_in_client, pid_client);
                    shmdt(shm);
                }
                
                create_message(out_msg, MSG_CF, logged_in_client);
                printf("[SERVER] Đăng nhập thành công: %s\n", logged_in_client);
            } else {
                // ❌ Tài khoản BLOCKED (status == '0')
                create_message(out_msg, MSG_DENY, "Account is blocked.");
                printf("[SERVER] Tài khoản bị khóa: %s\n", pending_username);
            }
        } else {
            // ❌ Password sai
            create_message(out_msg, MSG_DENY, "Incorrect password.");
        }
    }
    break;
}
```

## Test Cases

### Test 1: Tài khoản Active (status = '1') ✅
```
Username: hungng
Password: hung2a9
Status: 1

Kết quả:
[CLIENT] Enter password:
[CLIENT] Server CONFIRM (0x11): hungng
→ Đăng nhập THÀNH CÔNG
```

### Test 2: Tài khoản Blocked (status = '0') ❌
```
Username: namnk
Password: namnk123
Status: 0

Kết quả:
[CLIENT] Enter password:
[CLIENT] Server DENY (0x00): ERROR: Account is blocked.
→ Bị TỪ CHỐI do tài khoản bị khóa
```

### Test 3: Password sai ❌
```
Username: hungng
Password: wrongpassword

Kết quả:
[CLIENT] Server DENY (0x00): ERROR: Incorrect password.
→ Bị TỪ CHỐI do password sai
```

### Test 4: Username không tồn tại ❌
```
Username: invaliduser

Kết quả:
[CLIENT] Server DENY (0x00): ERROR: Account does not exist.
→ Bị TỪ CHỐI do không tìm thấy tài khoản
```

## Danh sách tài khoản trong account.txt

| Username | Password   | Email | Homepage | Status | Kết quả đăng nhập |
|----------|------------|-------|----------|--------|-------------------|
| admin    | admin123   | ...   | ...      | **0** | ❌ BLOCKED        |
| namnk    | namnk123   | ...   | ...      | **0** | ❌ BLOCKED        |
| hungng   | hung2a9    | ...   | ...      | **1** | ✅ ACTIVE         |
| locdinh  | loc2004    | ...   | ...      | **0** | ❌ BLOCKED        |
| quangvn  | vnquang12  | ...   | ...      | **1** | ✅ ACTIVE         |
| test2    | test123    | ...   | ...      | **1** | ✅ ACTIVE         |

## Flow đăng nhập với kiểm tra status

```
Client                          Server
  |                               |
  |--- MSG_LOGIN(username) ------>|
  |                               | 1. Check username exists ✓
  |<---- MSG_PASSWORD ------------|
  |                               |
  | (User inputs password)        |
  |--- MSG_PASSWORD(pass) -------->|
  |                               | 2. Check password ✓
  |                               | 3. Check status:
  |                               |    if status == '1':
  |                               |      → Add to shared memory
  |<---- MSG_CF(username) --------|      → Allow login ✅
  |                               |    else (status == '0'):
  |<---- MSG_DENY("blocked") -----|      → Deny login ❌
```

## Verification

### Kiểm tra trên Server:
```bash
tail -f server_output.log
```
Output mong đợi:
```
[SERVER]|[CHILD 1234] Nhận password cho username: namnk
[SERVER]|[CHILD 1234] Tài khoản bị khóa: namnk
```

### Kiểm tra trên Client:
```bash
./client 127.0.0.1 8080
```
Input:
```
namnk
namnk123
```
Output:
```
[CLIENT] Enter password:
[CLIENT] Server DENY (0x00): ERROR: Account is blocked.
```

## Lưu ý quan trọng

1. **Status là ký tự**: 
   - So sánh: `acc.status == '1'` (character '1')
   - KHÔNG phải: `acc.status == 1` (integer 1)

2. **Kiểm tra thứ tự**:
   - Bước 1: Username tồn tại?
   - Bước 2: Password đúng?
   - Bước 3: Status active?

3. **Shared Memory**:
   - Chỉ thêm client vào shared memory KHI status == '1'
   - Client bị blocked sẽ KHÔNG xuất hiện trong WHO command

## Kết luận

✅ Cơ chế đăng nhập đã được sửa lại hoàn chỉnh:
- Status '1' → Cho phép đăng nhập
- Status '0' → Từ chối với message "Account is blocked."
- Password sai → Từ chối với message "Incorrect password."
- Username không tồn tại → Từ chối với message "Account does not exist."
