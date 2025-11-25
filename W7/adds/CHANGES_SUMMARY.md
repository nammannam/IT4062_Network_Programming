# Tóm tắt các thay đổi - Thêm chức năng WHO command

## 1. Thay đổi trong `protocol.h`:
- Thêm `MSG_PASSWORD (0x03)`: Message type cho việc nhập password
- Thêm `MSG_WHO (0x02)`: Client yêu cầu danh sách users online
- Thêm `MSG_LIST (0x12)`: Server trả về danh sách users
- Thêm `LoggedInClient_s`: Struct lưu thông tin client đã login
- Thêm `SharedClientList_s`: Struct quản lý danh sách clients trong shared memory
- Thêm các functions: `init_shared_memory()`, `add_logged_in_client()`, `remove_logged_in_client()`, `get_logged_in_clients_list()`
- Thêm include: `<sys/shm.h>`, `<sys/ipc.h>`, `<sys/stat.h>`, `<sys/types.h>`

## 2. Thay đổi trong `protocol_utils.c`:
- Thêm biến global `pending_username[64]`: Lưu username đang chờ nhập password
- Implement `init_shared_memory()`: Khởi tạo shared memory segment
- Implement `add_logged_in_client()`: Thêm client vào danh sách shared memory
- Implement `remove_logged_in_client()`: Xóa client khỏi shared memory
- Implement `get_logged_in_clients_list()`: Lấy danh sách clients dạng string

### Cập nhật `server_handle_message()`:
- **MSG_LOGIN**: Kiểm tra username tồn tại trong account.txt, nếu có thì yêu cầu password
- **MSG_PASSWORD** (mới): Xác thực password, kiểm tra status tài khoản, thêm vào shared memory nếu thành công
- **MSG_WHO** (mới): Lấy danh sách logged-in clients từ shared memory và trả về MSG_LIST
- **MSG_TEXT**: Giữ nguyên logic cũ (lưu log)

### Cập nhật `client_handle_response()`:
- **MSG_PASSWORD**: Hiển thị prompt yêu cầu nhập password
- **MSG_LIST**: Hiển thị danh sách users online
- **MSG_CF**, **MSG_DENY**: Giữ nguyên

## 3. Thay đổi trong `server.c`:
- Trong `main()`: Khởi tạo shared memory và reset khi server start
- Trong `handle_client()`:
  - Khởi tạo shared memory connection
  - Xóa các logic đăng nhập cũ (đã chuyển vào protocol_utils.c)
  - Cleanup shared memory khi client disconnect
  - Gọi `remove_logged_in_client()` khi client logout hoặc disconnect

## 4. Thay đổi trong `client.c`:
- Cập nhật logic đăng nhập:
  - Gửi username (MSG_LOGIN)
  - Nếu nhận MSG_PASSWORD: nhập password và gửi lại
  - Nếu nhận MSG_CF: đăng nhập thành công
  - Nếu nhận MSG_DENY: hiển thị lỗi
- Thêm xử lý command "who":
  - Gửi MSG_WHO đến server
  - Nhận và hiển thị MSG_LIST (danh sách users)

## 5. Flow đăng nhập mới:
```
Client                          Server
  |                               |
  |--- MSG_LOGIN(username) ------>|
  |                               | (Check username exists)
  |<---- MSG_PASSWORD ------------|
  |                               |
  | (User inputs password)        |
  |--- MSG_PASSWORD(pass) -------->|
  |                               | (Validate password & status)
  |                               | (Add to shared memory if OK)
  |<---- MSG_CF(username) --------|  (Success)
  |   hoặc                        |
  |<---- MSG_DENY(error) ---------|  (Failed)
```

## 6. Flow WHO command:
```
Client                          Server
  |                               |
  | (User types "who")            |
  |--- MSG_WHO ------------------>|
  |                               | (Query shared memory)
  |<---- MSG_LIST(users) ---------|
  |                               |
  | (Display: "user1, user2,...")  |
```

## 7. Shared Memory Management:
- **Key**: 0x1234
- **Size**: sizeof(SharedClientList_s)
- **Permissions**: 0666
- **Cleanup**: Server reset khi khởi động, clients removed khi logout/disconnect

## 8. Tài khoản test (từ account.txt):
- **Active (status='1')**: hungng, quangvn, test2
- **Blocked (status='0')**: admin, namnk, locdinh

## 9. Commands có sẵn sau khi login:
- `who`: Xem danh sách users online
- `bye`: Logout
- Bất kỳ text nào khác: Gửi text message (lưu vào log)

## 10. Files mới được tạo:
- `TEST_WHO_COMMAND.md`: Hướng dẫn test chi tiết
- `CHANGES_SUMMARY.md`: File này
