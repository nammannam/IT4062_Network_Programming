#ifndef LLIST_H
#define LLIST_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_LEN 20

/**
 * @struct AccountInfo
 * @brief Cấu trúc lưu thông tin tài khoản
 */
typedef struct AccountInfo {
    char username[MAX_LEN];   /**< Tên đăng nhập */
    char password[MAX_LEN];   /**< Mật khẩu */
    char email[MAX_LEN];      /**< Email */
    char homepage[MAX_LEN];   /**< Trang chủ */
    char status;              /**< Trạng thái tài khoản */
} AccountInfo_s;

/**
 * @struct Llist
 * @brief Cấu trúc node trong danh sách liên kết
 */
typedef struct Llist {
    AccountInfo_s nodeInfo;   /**< Thông tin tài khoản */
    struct Llist *next;       /**< Con trỏ đến node tiếp theo */
} Llist_s;

/* Function declarations */

/**
 * @brief Tạo một danh sách liên kết mới
 * @return Con trỏ đến danh sách mới được tạo
 */
Llist_s* newList(void);

/**
 * @brief Tạo một node mới với thông tin tài khoản
 * @param info Thông tin tài khoản cần lưu
 * @return Con trỏ đến node mới được tạo
 */
Llist_s* makeNewNode(AccountInfo_s info);

/**
 * @brief Chèn một node vào đầu danh sách
 * @param list Con trỏ đến danh sách
 * @param info Thông tin tài khoản cần chèn
 * @return Con trỏ đến danh sách sau khi chèn
 */
Llist_s* insertAtHead(Llist_s *list, AccountInfo_s info);

/**
 * @brief Chèn một node vào cuối danh sách
 * @param list Con trỏ đến danh sách
 * @param info Thông tin tài khoản cần chèn
 * @return Con trỏ đến danh sách sau khi chèn
 */
Llist_s* insertAtTail(Llist_s *list, AccountInfo_s info);

/**
 * @brief Tìm kiếm tài khoản theo username
 * @param list Con trỏ đến danh sách
 * @param username Username cần tìm
 * @return Thông tin tài khoản nếu tìm thấy, struct rỗng nếu không tìm thấy
 */
AccountInfo_s searchUsernameList(Llist_s *list, char *username);

/**
 * @brief Kiểm tra password của tài khoản
 * @param llist Con trỏ đến danh sách
 * @param account Thông tin tài khoản cần kiểm tra
 * @param password Password cần kiểm tra
 * @return Thông tin tài khoản nếu password đúng, struct rỗng nếu sai
 */
AccountInfo_s checkAccountPassword(Llist_s *llist, AccountInfo_s account, char password[]);

/**
 * @brief Tìm node theo username
 * @param list Con trỏ đến danh sách
 * @param username Username cần tìm
 * @return Con trỏ đến node nếu tìm thấy, NULL nếu không tìm thấy
 */
Llist_s* findNodeByUsername(Llist_s *list, char username[]);

/**
 * @brief Xóa node theo username
 * @param list Con trỏ đến danh sách
 * @param username Username của node cần xóa
 */
void deleteNodeByUsername(Llist_s *list, char username[]);

/**
 * @brief Xóa toàn bộ danh sách
 * @param list Con trỏ đến danh sách cần xóa
 */
void deleteList(Llist_s *list);

#endif /* LLIST_H */



