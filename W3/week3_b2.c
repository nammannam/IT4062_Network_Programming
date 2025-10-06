/*

Week 3 - Bài tập 2
Nguyen Khanh Nam - 20225749

*/


#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

// Kiểm tra xem có phải là địa chỉ IPv6 đặc biệt không
int is_special_ipv6(const char *ipv6_str) {
    struct in6_addr addr;
    if (inet_pton(AF_INET6, ipv6_str, &addr) <= 0) {
        return 0;
    }
    
    // Loopback IPv6 (::1)
    if (IN6_IS_ADDR_LOOPBACK(&addr)) {
        return 1;
    }
    
    // Link-local IPv6 (fe80::/10)
    if (IN6_IS_ADDR_LINKLOCAL(&addr)) {
        return 1;
    }
    
    // Site-local IPv6 (fec0::/10) - deprecated
    if (IN6_IS_ADDR_SITELOCAL(&addr)) {
        return 1;
    }
    
    // Multicast IPv6 (ff00::/8)
    if (IN6_IS_ADDR_MULTICAST(&addr)) {
        return 1;
    }
    
    // Unique local IPv6 (fc00::/7)
    if ((addr.s6_addr[0] & 0xfe) == 0xfc) {
        return 1;
    }
    
    return 0;
}

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

// Kiểm tra IPv6 hợp lệ
int is_valid_ipv6(const char *ipv6_str) {
    struct in6_addr addr;
    return inet_pton(AF_INET6, ipv6_str, &addr) > 0 ? 1 : 0;
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
            return 0; // Có ký tự khác ngoài số và dấu chấm 
        }
    }
    
    // Nếu không có số hoặc không có dấu chấm -
    if (!has_digit || !has_dot) {
        return 0;
    }
    

    // Kiểm tra độ dài
    if (len < 7 || len > 15) return 1; //IP không hợp lệ
    
    // Đếm số dấu chấm - phải có đúng 3 dấu chấm
    int dot_count = 0;
    for (int i = 0; i < len; i++) {
        if (ip_str[i] == '.') {
            dot_count++;
        }
    }

    if (dot_count != 3) return 1; //IP không hợp lệ

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
    struct addrinfo hints, *result, *rp;
    char ip_str[INET6_ADDRSTRLEN];
    
    // Ghi log query
    log_query(hostname, "HOSTNAME_TO_IP", "Querying...");
    
    // Cấu hình hints để lấy cả IPv4 và IPv6
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    // IPv4 hoặc IPv6
    hints.ai_socktype = SOCK_STREAM;
    
    int status = getaddrinfo(hostname, NULL, &hints, &result);
    
    if (status != 0) {
        printf("Not found information\n");
        log_query(hostname, "HOSTNAME_TO_IP", "Not found");
        return;
    }
    
    int ipv4_count = 0, ipv6_count = 0;
    char first_ip[INET6_ADDRSTRLEN] = "";
    
    // Duyệt qua tất cả địa chỉ IPv4 trước
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)rp->ai_addr;
            inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, INET_ADDRSTRLEN);
            if (ipv4_count == 0) {
                printf("Official IP: %s\n", ip_str);
                strcpy(first_ip, ip_str);
            }
            ipv4_count++;
        }
    }
    
    // Hiển thị Alias IP (các IPv4 addresses bổ sung)
    if (ipv4_count > 1) {
        printf("Alias IP:\n");
        int alias_count = 0;
        for (rp = result; rp != NULL; rp = rp->ai_next) {
            if (rp->ai_family == AF_INET) {
                struct sockaddr_in *ipv4 = (struct sockaddr_in *)rp->ai_addr;
                inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, INET_ADDRSTRLEN);
                if (alias_count > 0) {  // Bỏ qua IP đầu tiên (đã hiển thị là Official)
                    printf("%s\n", ip_str);
                }
                alias_count++;
            }
        }
    } else if (ipv4_count == 1) {
        printf("Alias IP:\n");  // Hiển thị dòng này ngay cả khi không có alias
    }
    
    // Hiển thị IPv6 addresses nếu có
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET6) {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)rp->ai_addr;
            inet_ntop(AF_INET6, &(ipv6->sin6_addr), ip_str, INET6_ADDRSTRLEN);
            if (ipv6_count == 0) {
                printf("Official IPv6: %s\n", ip_str);
                if (strlen(first_ip) == 0) strcpy(first_ip, ip_str);
            } else {
                printf("Alias IPv6: %s\n", ip_str);
            }
            ipv6_count++;
        }
    }
    
    // Ghi log kết quả đầu tiên
    log_query(hostname, "HOSTNAME_TO_IP", first_ip);
    
    freeaddrinfo(result);
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

void resolve_ipv6_to_hostname(const char *ipv6_str) {
    struct sockaddr_in6 addr;
    char hostname[NI_MAXHOST];
    
    // Kiểm tra IPv6 có hợp lệ không
    if (!is_valid_ipv6(ipv6_str)) {
        printf("Invalid IPv6 address\n");
        log_query(ipv6_str, "IPv6_TO_HOSTNAME", "Invalid address");
        return;
    }
    
    // Kiểm tra IPv6 đặc biệt và cảnh báo
    if (is_special_ipv6(ipv6_str)) {
        printf("special IPv6 address — may not have DNS record\n");
        log_query(ipv6_str, "IPv6_TO_HOSTNAME", "Special IPv6 - may not have DNS record");
    }
    
    // Chuẩn bị sockaddr_in6
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    
    if (inet_pton(AF_INET6, ipv6_str, &addr.sin6_addr) <= 0) {
        printf("Invalid IPv6 address\n");
        log_query(ipv6_str, "IPv6_TO_HOSTNAME", "Invalid address");
        return;
    }
    
    // Ghi log query
    log_query(ipv6_str, "IPv6_TO_HOSTNAME", "Querying...");
    
    // Sử dụng getnameinfo để reverse lookup IPv6
    int result = getnameinfo((struct sockaddr*)&addr, sizeof(addr), 
                            hostname, sizeof(hostname), NULL, 0, 0);
    
    if (result != 0) {
        printf("Not found information\n");
        log_query(ipv6_str, "IPv6_TO_HOSTNAME", "Not found");
        return;
    }
    
    // Hiển thị tên miền
    printf("Official name: %s\n", hostname);
    log_query(ipv6_str, "IPv6_TO_HOSTNAME", hostname);
}

void process_query(const char *query) {
    // Kiểm tra IPv6 trước
    if (is_ipv6(query)) {
        if (is_valid_ipv6(query)) {
            // IPv6 hợp lệ - thực hiện reverse lookup
            resolve_ipv6_to_hostname(query);
        } else {
            printf("Invalid IPv6 address\n");
            log_query(query, "INVALID_IPv6", "Invalid IPv6 address format");
        }
        return;
    }
    
    // Kiểm tra IPv4
    int ip_status = is_valid_ip(query);
    
    if (ip_status == 2) {
        // IPv4 hợp lệ - thực hiện reverse lookup
        resolve_ip_to_hostname(query);
    } else if (ip_status == 1) {
        // Trông giống IP nhưng không hợp lệ
        printf("Invalid address\n");
        log_query(query, "INVALID_IP", "Invalid address format");
    } else {
        // Không giống IP - coi là hostname
        resolve_hostname_to_ip(query);
    }
}

void process_batch_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Cannot open file %s\n", filename);
        return;
    }
    
    char line[256];
    int query_count = 0;
    
    printf("Processing batch file: %s\n", filename);
    printf("===================================\n");
    
    while (fgets(line, sizeof(line), file)) {
        // Loại bỏ ký tự newline
        line[strcspn(line, "\n")] = 0;
        
        // Bỏ qua dòng trống
        if (strlen(line) == 0) {
            continue;
        }
        
        query_count++;
        printf("\nQuery %d: %s\n", query_count, line);
        printf("Result:\n");
        process_query(line);
        printf("-----------------------------------\n");
    }
    
    fclose(file);
    printf("Total queries processed: %d\n", query_count);
}

void interactive_mode() {
    char input[256];
    printf("DNS Resolver - Interactive Mode\n");
    printf("Enter hostname or IP address (press Enter with empty line to exit):\n");
    printf("Supports: IPv4, IPv6, hostnames\n");
    printf("=============================================================\n\n");
    
    while (1) {
        printf("Query> ");
        fflush(stdout);
        
        // Đọc input từ user
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break; // EOF hoặc lỗi
        }
        
        // Loại bỏ ký tự newline
        input[strcspn(input, "\n")] = 0;
        
        // Kiểm tra chuỗi rỗng - thoát chương trình
        if (strlen(input) == 0) {
            break;
        }
        
        printf("Resolving: %s\n", input);
        printf("Result:\n");
        process_query(input);
        printf("\n"); // Thêm dòng trống giữa các query
    }
}

void print_usage(const char *program_name) {
    printf("Usage:\n");
    printf("  Interactive mode: %s\n", program_name);
    printf("  Single query:     %s <hostname_or_ip>\n", program_name);
    printf("  Batch mode:       %s -f <filename>\n", program_name);
    printf("\n");
    printf("=============================================================\n");
}

int main(int argc, char *argv[]) {
    // Nếu không có tham số - chạy chế độ interactive

    print_usage(argv[0]);

    if (argc == 1) {
        interactive_mode();
        return 0;
    }
    
    // Chế độ batch (-f filename)
    if (argc == 3 && strcmp(argv[1], "-f") == 0) {
        process_batch_file(argv[2]);
        return 0;
    }
    
    // Chế độ single query
    if (argc == 2) {
        char *parameter = argv[1];
        process_query(parameter);
        return 0;
    }
    
    // Tham số không hợp lệ
    print_usage(argv[0]);
    return 1;
}