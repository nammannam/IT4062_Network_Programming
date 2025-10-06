#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Kiểm tra xem có phải là địa chỉ IP đặc biệt không
int is_special_ip(const char *ip_str) {
    struct in_addr addr;
    if (inet_pton(AF_INET, ip_str, &addr) <= 0) {
        return 0;
    }
    
    unsigned long ip = ntohl(addr.s_addr);
    
    // Loopback (127.0.0.0/8)
    if ((ip & 0xFF000000) == 0x7F000000) {
        return 1;
    }
    
    // Private addresses
    // 10.0.0.0/8
    if ((ip & 0xFF000000) == 0x0A000000) {
        return 1;
    }
    // 172.16.0.0/12
    if ((ip & 0xFFF00000) == 0xAC100000) {
        return 1;
    }
    // 192.168.0.0/16
    if ((ip & 0xFFFF0000) == 0xC0A80000) {
        return 1;
    }
    
    // Multicast (224.0.0.0/4)
    if ((ip & 0xF0000000) == 0xE0000000) {
        return 1;
    }
    
    // Link-local (169.254.0.0/16)
    if ((ip & 0xFFFF0000) == 0xA9FE0000) {
        return 1;
    }
    
    return 0;
}

// Hàm ghi log
void log_query(const char *query, const char *result_type, const char *result) {
    FILE *log_file = fopen("resolver.log", "a");
    if (log_file != NULL) {
        time_t now = time(NULL);
        char *time_str = ctime(&now);
        time_str[strlen(time_str) - 1] = '\0'; // Bỏ ký tự newline
        
        fprintf(log_file, "[%s] Query: %s | Type: %s | Result: %s\n", 
                time_str, query, result_type, result);
        fclose(log_file);
    }
}

// Kiểm tra IPv6
int is_ipv6(const char *str) {
    return strchr(str, ':') != NULL;
}
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
    
    // Ghi log query
    log_query(hostname, "HOSTNAME_TO_IP", "Querying...");
    
    host_entry = gethostbyname(hostname);
    
    if (host_entry == NULL) {
        printf("Not found information\n");
        log_query(hostname, "HOSTNAME_TO_IP", "Not found");
        return;
    }
    
    // Hiển thị IP chính thức (IP đầu tiên)
    addr.s_addr = *((unsigned long *)host_entry->h_addr_list[0]);
    char official_ip[16];
    strcpy(official_ip, inet_ntoa(addr));
    printf("Official IP: %s\n", official_ip);
    
    // Ghi log kết quả
    log_query(hostname, "HOSTNAME_TO_IP", official_ip);
    
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
    int ip_check = is_valid_ip(ip_str);
    if (ip_check != 2) {
        printf("Invalid address\n");
        log_query(ip_str, "IP_TO_HOSTNAME", "Invalid address");
        return;
    }
    
    // Kiểm tra IP đặc biệt và cảnh báo
    if (is_special_ip(ip_str)) {
        printf("special IP address — may not have DNS record\n");
        log_query(ip_str, "IP_TO_HOSTNAME", "Special IP - may not have DNS record");
    }
    
    addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip_str, &addr.sin_addr) <= 0) {
        printf("Invalid address\n");
        log_query(ip_str, "IP_TO_HOSTNAME", "Invalid address");
        return;
    }
    
    // Ghi log query
    log_query(ip_str, "IP_TO_HOSTNAME", "Querying...");
    
    host_entry = gethostbyaddr(&addr.sin_addr, sizeof(addr.sin_addr), AF_INET);
    
    if (host_entry == NULL) {
        printf("Not found information\n");
        log_query(ip_str, "IP_TO_HOSTNAME", "Not found");
        return;
    }
    
    // Hiển thị tên miền chính thức
    printf("Official name: %s\n", host_entry->h_name);
    log_query(ip_str, "IP_TO_HOSTNAME", host_entry->h_name);
    
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
    
    // Kiểm tra IPv6 trước
    if (is_ipv6(parameter)) {
        printf("IPv6 is not supported\n");
        log_query(parameter, "IPv6", "Not supported");
        return 1;
    }
    
    // Kiểm tra tham số đầu vào
    int ip_status = is_valid_ip(parameter);
    
    if (ip_status == 2) {
        // IP hợp lệ - thực hiện reverse lookup
        resolve_ip_to_hostname(parameter);
    } else if (ip_status == 1) {
        // Trông giống IP nhưng không hợp lệ
        printf("Invalid address\n");
        log_query(parameter, "INVALID_IP", "Invalid address format");
    } else {
        // Không giống IP - coi là hostname
        resolve_hostname_to_ip(parameter);
    }
    
    return 0;
}