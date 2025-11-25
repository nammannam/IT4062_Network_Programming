# Hướng dẫn Test chức năng WHO command

## Tính năng đã implement:
1. **Đăng nhập cải tiến**: Client gửi username -> Server yêu cầu password -> Client gửi password -> Server xác thực
2. **WHO command**: Hiển thị danh sách các client đã đăng nhập vào server
3. **Shared Memory**: Sử dụng shared memory để tracking clients giữa các child process

## Các Message Type mới:
- `MSG_PASSWORD (0x03)`: Server yêu cầu client nhập password
- `MSG_WHO (0x02)`: Client yêu cầu danh sách users đang online
- `MSG_LIST (0x12)`: Server trả về danh sách users

## Hướng dẫn test:

### 1. Start server:
```bash
./server 8080
```

### 2. Mở 3 terminal khác để chạy 3 clients:

**Terminal Client 1:**
```bash
./client 127.0.0.1 8080
```
Nhập:
- `hungng` (username)
- `hung2a9` (password)
- `who` (xem danh sách - sẽ thấy: hungng)
- Giữ terminal này mở

**Terminal Client 2:**
```bash
./client 127.0.0.1 8080
```
Nhập:
- `quangvn` (username)
- `vnquang12` (password)
- `who` (xem danh sách - sẽ thấy: hungng, quangvn)
- Giữ terminal này mở

**Terminal Client 3:**
```bash
./client 127.0.0.1 8080
```
Nhập:
- `test2` (username)
- `test123` (password)
- `who` (xem danh sách - sẽ thấy: hungng, quangvn, test2)

### 3. Test logout và WHO:
Từ Client 1, nhập:
- `bye` (logout)

Từ Client 2 hoặc 3, nhập:
- `who` (sẽ chỉ thấy: quangvn, test2 - hungng đã logout)

### 4. Test text messaging:
Sau khi đăng nhập, gõ bất kỳ text nào (không phải "who" hoặc "bye"):
- `Hello from client!`
- Server sẽ lưu vào file log

### 5. Test tài khoản bị khóa (status = '0'):
```bash
./client 127.0.0.1 8080
```
Nhập:
- `admin` (username)
- `admin123` (password)
- Sẽ nhận message: "Account is blocked."

### 6. Test tài khoản không tồn tại:
```bash
./client 127.0.0.1 8080
```
Nhập:
- `invalid_user`
- Sẽ nhận message: "Account does not exist."

### 7. Test password sai:
```bash
./client 127.0.0.1 8080
```
Nhập:
- `hungng`
- `wrongpassword`
- Sẽ nhận message: "Incorrect password."

## Kết quả mong đợi:
- ✅ Đăng nhập yêu cầu cả username và password
- ✅ WHO command hiển thị tất cả users đang online
- ✅ Khi client logout, họ biến mất khỏi danh sách WHO
- ✅ Multiple clients có thể đăng nhập đồng thời
- ✅ Text messages được log vào file ./log/user.txt
- ✅ Tài khoản bị khóa không thể đăng nhập
- ✅ Password sai bị từ chối

## Kiểm tra Shared Memory:
```bash
# Xem shared memory segments
ipcs -m

# Xóa shared memory (nếu cần cleanup)
ipcrm -M 0x1234
```

## Log file:
Check file log sau khi gửi text messages:
```bash
cat ./log/user.txt
```
