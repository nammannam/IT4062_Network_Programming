/*

Week 3 - Bài tập 1
Nguyen Khanh Nam - 20225749

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

// Trả về: 2 = IP hợp lệ, 1 = giống IP nhưng không hợp lệ, 0 = không giống IP
int is_valid_ip(const char *ip_str) {
    // Kiểm tra xem chuỗi có chứa chỉ số và dấu chấm không
    int has_digit = 0;
    int has_dot = 0;
    int len = strlen(ip_str);
    
    for (int i = 0; i < len; i++) {
        if (ip_str[i] >= '0' && ip_str[i] <= '9') {
            has_digit = 1;
        } else if (ip_str[i] == '.') {
            has_dot = 1;
        } else {
            return 0; // Có ký tự khác ngoài số và dấu chấm -> không giống IP
        }
    }
    
    // Nếu không có số hoặc không có dấu chấm -> không giống IP
    if (!has_digit || !has_dot) {
        return 0;
    }
    
    // Từ đây trở đi, chuỗi "trông giống IP" (chỉ có số và dấu chấm)
    // Bây giờ kiểm tra xem có hợp lệ không
    
    // Kiểm tra độ dài
    if (len < 7 || len > 15) return 1; // Trông giống IP nhưng không hợp lệ
    
    // Đếm số dấu chấm - phải có đúng 3 dấu chấm
    int dot_count = 0;
    for (int i = 0; i < len; i++) {
        if (ip_str[i] == '.') {
            dot_count++;
        }
    }
    
    if (dot_count != 3) return 1; // Trông giống IP nhưng không hợp lệ
    
    // Kiểm tra không được bắt đầu hoặc kết thúc bằng dấu chấm
    if (ip_str[0] == '.' || ip_str[len-1] == '.') return 1;
    
    // Kiểm tra không có hai dấu chấm liên tiếp
    for (int i = 0; i < len - 1; i++) {
        if (ip_str[i] == '.' && ip_str[i+1] == '.') return 1;
    }
    
    // Parse và kiểm tra từng octet
    int a, b, c, d;
    char extra; // để kiểm tra có ký tự thừa không
    int count = sscanf(ip_str, "%d.%d.%d.%d%c", &a, &b, &c, &d, &extra);
    
    if (count != 4) return 1; // Phải đọc được đúng 4 số, không có ký tự thừa
    
    // Kiểm tra từng octet phải trong khoảng 0-255
    if (a < 0 || a > 255 || b < 0 || b > 255 || 
        c < 0 || c > 255 || d < 0 || d > 255) {
        return 1;
    }
    
    // Kiểm tra format chính xác bằng cách so sánh với chuỗi được format lại
    char formatted[16];
    snprintf(formatted, sizeof(formatted), "%d.%d.%d.%d", a, b, c, d);
    if (strcmp(ip_str, formatted) != 0) return 1;
    
    return 2; // IP hợp lệ
}

void resolve_hostname_to_ip(const char *hostname) {
    struct hostent *host_entry;
    struct in_addr addr;
    
    host_entry = gethostbyname(hostname);
    
    if (host_entry == NULL) {
        printf("Not found information\n");
        return;
    }
    
    // Hiển thị IP chính thức (IP đầu tiên)
    addr.s_addr = *((unsigned long *)host_entry->h_addr_list[0]);
    printf("Official IP: %s\n", inet_ntoa(addr));
    
    // Hiển thị các IP phụ (alias IP)
    printf("Alias IP:\n");
    int i = 1;
    while (host_entry->h_addr_list[i] != NULL) {
        addr.s_addr = *((unsigned long *)host_entry->h_addr_list[i]);
        printf("%s\n", inet_ntoa(addr));
        i++;
    }
}

void resolve_ip_to_hostname(const char *ip_str) {
    struct sockaddr_in addr;
    struct hostent *host_entry;
    
    // Kiểm tra IP có hợp lệ không
    if (!is_valid_ip(ip_str)) {
        printf("Invalid address\n");
        return;
    }
    
    addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip_str, &addr.sin_addr) <= 0) {
        printf("Invalid address\n");
        return;
    }
    
    host_entry = gethostbyaddr(&addr.sin_addr, sizeof(addr.sin_addr), AF_INET);
    
    if (host_entry == NULL) {
        printf("Not found information\n");
        return;
    }
    
    // Hiển thị tên miền chính thức
    printf("Official name: %s\n", host_entry->h_name);
    
    // Hiển thị các tên miền phụ (alias)
    printf("Alias name:\n");
    int i = 0;
    while (host_entry->h_aliases[i] != NULL) {
        printf("%s\n", host_entry->h_aliases[i]);
        i++;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <hostname_or_ip>\n", argv[0]);
        return 1;
    }
    
    char *parameter = argv[1];
    
    // Kiểm tra tham số đầu vào
    int ip_status = is_valid_ip(parameter);
    
    if (ip_status == 2) {
        // IP hợp lệ - thực hiện reverse lookup
        resolve_ip_to_hostname(parameter);
    } else if (ip_status == 1) {
        // Trông giống IP nhưng không hợp lệ
        printf("Invalid address\n");
    } else {
        // Không giống IP - coi là hostname
        resolve_hostname_to_ip(parameter);
    }
    
    return 0;
}