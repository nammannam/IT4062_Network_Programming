# Testing Multiple Clients with Non-Blocking Server

## Đã thêm Debug Output

Server bây giờ có thêm nhiều debug output để tracking:
- Số lượng clients hiện tại
- Khi có connection mới
- Khi client gửi data
- Khi client disconnect

## Cách Test Multiple Clients

### Method 1: Manual Testing (Khuyến nghị)

**Terminal 1 - Start Server:**
```bash
./server 8787
```

**Terminal 2, 3, 4 - Start Clients:**
```bash
./client_nonblocking 127.0.0.1 8787
```

Mở 3-4 terminal và chạy client, sau đó thử đăng nhập với các tài khoản khác nhau:
- `admin` / `admin123`
- `namnk` / `namnk123`
- `quangvn` / `vnquang12`
- `test2` / `test123`

### Method 2: Using Test Script

```bash
# Terminal 1: Start server
./server 8787

# Terminal 2: Run test script
./test_clients.sh
```

## Những gì cần quan sát

### Trên Server Terminal:

Bạn sẽ thấy:
```
[DEBUG] Waiting for activity (max_fd=3, client_count=0)...
[DEBUG] select() returned activity=1
[DEBUG] New connection detected on listenfd
[DEBUG] Accepted connfd=4
Client added at index 0 (sockfd=4)
Accepted connection from 127.0.0.1:xxxxx (sockfd=4, index=0, total=1)
[DEBUG] Waiting for activity (max_fd=4, client_count=1)...
```

Khi client thứ 2 kết nối:
```
[DEBUG] select() returned activity=1
[DEBUG] New connection detected on listenfd
[DEBUG] Accepted connfd=5
Client added at index 1 (sockfd=5)
Accepted connection from 127.0.0.1:xxxxx (sockfd=5, index=1, total=2)
```

### Trên Client Terminal:

```
Connected to server (127.0.0.1:8787)
Client running in non-blocking mode with select()
Enter username to login:
admin

[CLIENT] Enter password:
admin123

[CLIENT] Hello admin
```

## Troubleshooting

### Nếu client thứ 2 không kết nối được:

1. **Kiểm tra log server** - Xem có thông báo "[DEBUG] New connection detected" không?

2. **Kiểm tra max_fd** - Nếu max_fd không tăng, có thể có bug trong code

3. **Test với netcat:**
```bash
# Terminal 1: Start server
./server 8787

# Terminal 2-4: Test connections
nc -v 127.0.0.1 8787
```

Nếu netcat có thể kết nối nhiều lần → vấn đề ở client
Nếu netcat chỉ kết nối 1 lần → vấn đề ở server

### Common Issues:

1. **Server chỉ nhận 1 client:**
   - Check xem có đang block ở `recv()` không
   - Check `select()` có return đúng không
   - Check FD_SET/FD_ISSET logic

2. **Client không gửi được message:**
   - Check `waitingForPassword` flag
   - Check message type (LOGIN vs PASSWORD)

3. **Connection timeout:**
   - Check firewall
   - Check port đã được bind đúng chưa

## Debug Commands

```bash
# Check listening ports
netstat -tlnp | grep 8787

# Check active connections
netstat -an | grep 8787

# Check process
ps aux | grep server
```

## Expected Behavior

✅ Server có thể accept nhiều clients đồng thời (up to 100)
✅ Mỗi client có thể login độc lập
✅ Server xử lý messages từ tất cả clients
✅ `who` command shows tất cả logged-in users
✅ Khi 1 client disconnect, các clients khác vẫn hoạt động bình thường

## Thành công khi:

- Có thể mở 3+ client terminals đồng thời
- Tất cả clients đều login được
- Command `who` shows tất cả users
- Server không crash khi client disconnect
- Logs hiển thị multiple active connections
