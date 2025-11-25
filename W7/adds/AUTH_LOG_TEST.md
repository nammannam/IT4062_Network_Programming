# Authentication Logging Test Guide

## Overview
The authentication logging system tracks all login and logout activities in `./log/auth.log` with timestamps.

## Log Format

### Successful Login
```
[timestamp] LOGIN username from IP:PORT SUCCESS
```

### Failed Login (Wrong Password)
```
[timestamp] LOGIN username from IP:PORT FAIL (wrong password)
```

### Account Locked
```
[timestamp] ACCOUNT_LOCKED username
[timestamp] LOGIN username from IP:PORT FAIL (account locked)
```

### Logout
```
[timestamp] LOGOUT username from IP:PORT
```

## Test Scenarios

### Test 1: Successful Login
**Accounts to test (status='1'):**
- hungng / hung2a9
- quangvn / vnquang12
- test2 / test123

**Steps:**
1. Start server: `./server 8080`
2. Start client: `./client 127.0.0.1 8080`
3. Enter username: `hungng`
4. Enter password: `hung2a9`

**Expected auth.log entry:**
```
[2025-11-25 XX:XX:XX] LOGIN hungng from 127.0.0.1:XXXXX SUCCESS
```

### Test 2: Failed Login - Wrong Password
**Steps:**
1. Start client: `./client 127.0.0.1 8080`
2. Enter username: `hungng`
3. Enter wrong password: `wrongpass`

**Expected auth.log entries:**
```
[2025-11-25 XX:XX:XX] LOGIN hungng from 127.0.0.1:XXXXX FAIL (wrong password)
```

### Test 3: Account Locked
**Accounts to test (status='0'):**
- admin / admin123
- namnk / namnk123
- locdinh / loc2004

**Steps:**
1. Start client: `./client 127.0.0.1 8080`
2. Enter username: `admin`
3. Enter correct password: `admin123`

**Expected auth.log entries:**
```
[2025-11-25 XX:XX:XX] ACCOUNT_LOCKED admin
[2025-11-25 XX:XX:XX] LOGIN admin from 127.0.0.1:XXXXX FAIL (account locked)
```

### Test 4: Logout (bye command)
**Steps:**
1. Login successfully (see Test 1)
2. Type: `bye`

**Expected auth.log entry:**
```
[2025-11-25 XX:XX:XX] LOGOUT hungng from 127.0.0.1:XXXXX
```

### Test 5: Logout (unexpected disconnect)
**Steps:**
1. Login successfully
2. Press Ctrl+C to force disconnect client

**Expected auth.log entry:**
```
[2025-11-25 XX:XX:XX] LOGOUT hungng from 127.0.0.1:XXXXX
```

## Complete Test Session Example

```bash
# Terminal 1 - Server
./server 8080

# Terminal 2 - Client 1
./client 127.0.0.1 8080
# Test account locked
admin
admin123
# Should see: "Account is blocked."

# Test successful login
hungng
hung2a9
# Should see: "Server CONFIRM (0x11): hungng"

# Send some messages
hello
who
# Should see logged-in users

# Logout
bye

# Terminal 3 - Client 2 (test concurrent logins)
./client 127.0.0.1 8080
# Test wrong password
quangvn
wrongpassword
# Should see: "Server DENY (0x00): ERROR: Incorrect password."

# Test successful login
quangvn
vnquang12
# Should see: "Server CONFIRM (0x11): quangvn"

# Test WHO command
who
# Should see: "Logged-in users: hungng, quangvn" (if hungng still logged in)

# Test unexpected disconnect
^C  # Press Ctrl+C
```

## Checking the Log File

After running tests, check the authentication log:

```bash
cat ./log/auth.log
```

**Expected output format:**
```
[2025-11-25 14:30:15] LOGIN admin from 127.0.0.1:54321 FAIL (account locked)
[2025-11-25 14:30:15] ACCOUNT_LOCKED admin
[2025-11-25 14:30:45] LOGIN hungng from 127.0.0.1:54322 SUCCESS
[2025-11-25 14:31:20] LOGOUT hungng from 127.0.0.1:54322
[2025-11-25 14:32:10] LOGIN quangvn from 127.0.0.1:54323 FAIL (wrong password)
[2025-11-25 14:32:30] LOGIN quangvn from 127.0.0.1:54323 SUCCESS
[2025-11-25 14:33:00] LOGOUT quangvn from 127.0.0.1:54323
```

## Implementation Details

### Files Modified

1. **protocol.h**
   - Added authentication logging function declarations
   - Functions: `log_login_success`, `log_login_fail`, `log_account_locked`, `log_logout`

2. **protocol_utils.c**
   - Implemented authentication logging functions with timestamp formatting
   - Integrated logging calls in MSG_PASSWORD handler
   - Logs created in `./log/auth.log`

3. **server.c**
   - Added client IP and port tracking
   - Updated `server_handle_message` call with IP/port parameters
   - Added logout logging on disconnect and "bye" command

### Log File Location
- Path: `./log/auth.log`
- Directory auto-created if it doesn't exist
- File opened in append mode (previous logs preserved)

### Timestamp Format
- Format: `YYYY-MM-DD HH:MM:SS`
- Uses local time (`localtime()`)

## Verification Checklist

- [ ] Successful login creates SUCCESS log entry
- [ ] Wrong password creates FAIL (wrong password) entry
- [ ] Blocked account creates ACCOUNT_LOCKED and FAIL (account locked) entries
- [ ] "bye" command creates LOGOUT entry
- [ ] Unexpected disconnect (Ctrl+C) creates LOGOUT entry
- [ ] Multiple clients logged in simultaneously all logged correctly
- [ ] Timestamps are accurate
- [ ] IP addresses and ports are correct
- [ ] Log file persists across server restarts
