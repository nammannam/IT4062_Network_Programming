# TEST: Account Locking - File AND List Update

## Vấn đề đã fix
**BUG TRƯỚC ĐÓ**: Khi lock tài khoản sau 3 lần sai password, chỉ cập nhật `account.txt` file nhưng không cập nhật List trong memory. Kết quả: Server vẫn cho login vì List vẫn có status='1'.

**FIX**: Thêm function `lock_account_in_list()` để cập nhật **CẢ FILE VÀ LIST**.

## Changes Made

### 1. protocol.h
```c
void lock_account_in_list(Llist_s *list, const char *username);
```

### 2. protocol_utils.c

#### New Function: lock_account_in_list()
```c
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
```

#### Updated MSG_PASSWORD Handler
```c
if (*attempts >= 3) {
    // Lock account after 3 failed attempts
    lock_account_in_file(pending_username, "./account.txt");
    lock_account_in_list(client_list, pending_username);  // <--- NEW LINE
    
    create_message(out_msg, MSG_DENY, "Account locked due to too many failed login attempts.");
    // ... rest of code
}
```

## Test Cases

### Test Case 1: Tài khoản đã bị khóa sẵn (status='0')
**Setup**: `namnk 123 namnk@soictmail.com soict.hust.edu.vn 0`

**Steps**:
1. Start server
2. Client login với username: `namnk`
3. Enter password: `123` (đúng password)

**Expected Result**:
- Server response: `MSG_DENY` với message "Account is blocked."
- Console log: `[SERVER]|[CHILD xxx] Tài khoản bị khóa: namnk`

---

### Test Case 2: Lock tài khoản sau 3 lần sai password
**Setup**: Reset `hungng` về status='1' trong `account.txt`:
```
hungng 123 hungng@soictmail.com soict.hust.edu.vn 1
```

**Steps**:
1. Start server
2. Client login với username: `hungng`
3. Enter password: `wrong1` (sai) - Attempt 1/3
4. Server response: `MSG_DENY` với "Invalid username or password."
5. Login lại username: `hungng`
6. Enter password: `wrong2` (sai) - Attempt 2/3
7. Server response: `MSG_DENY` với "Invalid username or password."
8. Login lại username: `hungng`
9. Enter password: `wrong3` (sai) - Attempt 3/3
10. Server response: `MSG_DENY` với "Account locked due to too many failed login attempts."

**Expected Server Console Output**:
```
[SERVER]|[CHILD xxx] Password sai cho: hungng (Attempts: 1/3)
[SERVER]|[CHILD xxx] Password sai cho: hungng (Attempts: 2/3)
[SERVER]|[CHILD xxx] Password sai cho: hungng (Attempts: 3/3)
[LOCK ACCOUNT] Locked account: hungng (changed status to 0)
[LOCK ACCOUNT IN LIST] Locked account in memory: hungng (status = 0)
[SERVER]|[CHILD xxx] Tài khoản bị khóa do đăng nhập sai quá 3 lần: hungng
```

**Verify**:
1. Check `account.txt` - hungng status should be '0'
2. Check `log/auth.log` - should have ACCOUNT_LOCKED log
3. **CRITICAL**: Try login again with username `hungng` và password `123` (CORRECT password)
   - Should get `MSG_DENY` với "Account is blocked."
   - This proves List was updated, not just file!

---

### Test Case 3: Verify List update persists in memory
**Setup**: After Test Case 2 (hungng locked)

**Steps**:
1. **DO NOT restart server** (same server process)
2. Client login với username: `hungng`
3. Enter password: `123` (CORRECT password)

**Expected Result**:
- Server response: `MSG_DENY` với "Account is blocked."
- Console log: `[SERVER]|[CHILD xxx] Tài khoản bị khóa: hungng`

**Why this test is important**:
- Proves that `lock_account_in_list()` successfully updated the List
- Without the fix, server would check List (still status='1') and allow login
- With fix, server checks List (now status='0') and blocks login

---

## Verification Commands

### Check account.txt status
```bash
cat account.txt | grep hungng
# Should show: hungng 123 hungng@soictmail.com soict.hust.edu.vn 0
```

### Check auth.log for lock event
```bash
tail -20 log/auth.log | grep ACCOUNT_LOCKED
# Should show: [2025-01-XX XX:XX:XX] ACCOUNT_LOCKED | hungng | reason: Too many failed login attempts
```

### Reset hungng for retesting
```bash
sed -i 's/hungng 123 hungng@soictmail.com soict.hust.edu.vn 0/hungng 123 hungng@soictmail.com soict.hust.edu.vn 1/' account.txt
```

---

## Implementation Summary

**Before Fix**:
```
Failed login 3 times → lock_account_in_file() → Update account.txt only
                                              ↓
                                         List still has status='1'
                                              ↓
                                    Next login: searchUsernameList() returns status='1'
                                              ↓
                                         LOGIN ALLOWED (BUG!)
```

**After Fix**:
```
Failed login 3 times → lock_account_in_file() → Update account.txt
                    ↓
                    → lock_account_in_list() → Update List to status='0'
                                              ↓
                                    Next login: searchUsernameList() returns status='0'
                                              ↓
                                         LOGIN BLOCKED (CORRECT!)
```

## Key Learning
**Data Consistency**: Khi có 2 nguồn dữ liệu (File + Memory), phải cập nhật **CẢ HAI** cùng lúc để tránh inconsistency!
