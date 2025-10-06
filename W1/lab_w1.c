#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>
#include "llist.h"

#define INPUT_FILE_PATH ".\\account.txt"
#define HISTORY_FILE_PATH ".\\history.txt"
#define MAX_LEN 20

int isLoggedIn = 0;         // 0 - no user logged in, 1 - user logged in
int adminMode = 0;          // 0 - normal user mode, 1 - admin mode
AccountInfo_s loggedInUser; // Current logged-in user

void clearInputBuffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Hàm xử lý dấu cách thừa ở đầu và cuối chuỗi
void trimSpaces(char *str)
{
    int len = strlen(str);
    int start = 0, end = len - 1;
    
    // Loại bỏ dấu cách ở đầu
    while (start < len && isspace(str[start]))
        start++;
    
    // Loại bỏ dấu cách ở cuối
    while (end >= start && isspace(str[end]))
        end--;
    
    // Dịch chuyển chuỗi về đầu
    int i;
    for (i = 0; i <= end - start; i++)
        str[i] = str[start + i];
    
    str[i] = '\0'; // Kết thúc chuỗi
}

// Hàm xử lý dấu cách thừa ở giữa chuỗi (chuyển nhiều dấu cách thành 1)
void normalizeSpaces(char *str)
{
    int len = strlen(str);
    int writeIndex = 0;
    int spaceFound = 0;
    
    for (int readIndex = 0; readIndex < len; readIndex++)
    {
        if (isspace(str[readIndex]))
        {
            if (!spaceFound)
            {
                str[writeIndex++] = ' '; // Chỉ giữ lại 1 dấu cách
                spaceFound = 1;
            }
            // Bỏ qua các dấu cách tiếp theo
        }
        else
        {
            str[writeIndex++] = str[readIndex];
            spaceFound = 0;
        }
    }
    str[writeIndex] = '\0';
}

// Hàm xử lý toàn bộ dấu cách (đầu, cuối và giữa)
void processSpaces(char *str)
{
    trimSpaces(str);        // Xử lý dấu cách đầu và cuối
    normalizeSpaces(str);   // Xử lý dấu cách ở giữa
}

// Hàm loại bỏ TẤT CẢ dấu cách khỏi chuỗi
void removeAllSpaces(char *str)
{
    int writeIndex = 0;
    int len = strlen(str);
    
    for (int readIndex = 0; readIndex < len; readIndex++)
    {
        if (!isspace(str[readIndex]))
        {
            str[writeIndex++] = str[readIndex];
        }
        // Bỏ qua tất cả các ký tự khoảng trắng
    }
    str[writeIndex] = '\0';
}

// Hàm nhập chuỗi có xử lý dấu cách thừa
void getInputWithTrim(char *prompt, char *buffer, int maxLen)
{
    printf("%s", prompt);
    // fgets(buffer, maxLen, stdin);
    scanf("%s", buffer);
    clearInputBuffer();
    // Xóa ký tự xuống dòng nếu có
    int len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
        buffer[len - 1] = '\0';
    
    // Xử lý tất cả dấu cách thừa (đầu, cuối và giữa)
    processSpaces(buffer);
}

// Hàm nhập chuỗi không cho phép dấu cách (dành cho username, password)
void getInputNoSpaces(char *prompt, char *buffer, int maxLen)
{
    printf("%s", prompt);
    // fgets(buffer, maxLen, stdin);
    scanf("%s", buffer);
    clearInputBuffer();
    // Xóa ký tự xuống dòng nếu có
    int len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
        buffer[len - 1] = '\0';
    
    // Tự động loại bỏ TẤT CẢ dấu cách
    removeAllSpaces(buffer);
}

// Hàm nhập chuỗi tự động loại bỏ tất cả dấu cách (cho phone number)
void getInputAutoRemoveSpaces(char *prompt, char *buffer, int maxLen)
{
    printf("%s", prompt);
    // fgets(buffer, maxLen, stdin);
    scanf("%s", buffer);
    clearInputBuffer();
    // Xóa ký tự xuống dòng nếu có
    int len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
        buffer[len - 1] = '\0';
    
    // Tự động loại bỏ TẤT CẢ dấu cách
    removeAllSpaces(buffer);
}

void menu()
{
    printf("========= User Management Program =========\n");
    if (isLoggedIn)
        printf("User session: %s\n", loggedInUser.username);
    printf("1. Register\n");
    printf("2. Sign in\n");
    printf("3. Change password\n");
    printf("4. Update account info\n");
    printf("5. Reset program\n");
    printf("6. View login history\n");
    printf("7. Sign out\n");
    printf("Your choice (1-7, other to quit): ");
}

void adminMenu()
{
    printf("========= Admin Menu =========\n");
    printf("1. View all accounts\n");
    printf("2. Delete an account\n");
    printf("3. Reset password for accounts\n");
    printf("Your choice (1-3, other to quit Admin mode): ");
}

bool checkPhoneNumber(char phone[])
{
    if (strlen(phone) != 10)
    {
        printf("So dien thoai phai co du 10 chu so\n");
        return false;
    }
    if (phone[0] != '0')
    {
        printf("So dien thoai phai bat dau bang so 0\n");
        return false;
    }
    for (int i = 1; i < strlen(phone); i++)
    {
        if (!isdigit(phone[i]))
        {
            printf("So dien thoai khong hop le\n");
            return false;
        }
    }
    return true;
}

bool checkEmail(char email[])
{
    int atCount = 0;
    int dotCount = 0;
    int atPosition = -1;
    int dotPosition = -1;
    for (int i = 0; i < strlen(email); i++)
    {
        if (email[i] == '@')
        {
            atCount++;
            atPosition = i;
        }
        if (email[i] == '.')
        {
            dotCount++;
            dotPosition = i;
        }

        if (dotCount > 1 || atCount > 1)
        {
            printf("Email chi co 1 ky tu ' @ ' va 1 ky tu ' . '\n");
            return false;
        }

        if (i > atPosition && atCount > 0)
        {
            if (!isalnum(email[i]) && email[i] != '.' && email[i] != '-' && email[i] != '_')
            {
                printf("Email khong hop le ky tu sau ' @ '\n");
                return false;
            }
        }
    }

    // @ khong duoc o dau va cuoi
    if (atPosition == 0 || atPosition == strlen(email) - 1 || atCount != 1)
    {
        printf("Email khong hop le ky tu ' @ '\n");
        return false;
    }
    // . khong duoc o dau va cuoi
    if (dotPosition == 0 || dotPosition == strlen(email) - 1 || dotCount != 1)
    {
        printf("Email khong hop le ky tu ' . '\n");
        return false;
    }
    // @ phai o truoc . va khong duoc gan nhau
    if (dotPosition < atPosition || abs(dotPosition - atPosition) == 1)
    {
        printf("Email khong hop le ky tu ' . ' va ' @ '\n");
        return false;
    }

    return true;
}

void readAccountsFromFile(char *filePath, Llist_s *list)
{
    FILE *fptr = fopen(filePath, "r");
    if (fptr == NULL)
    {
        printf("File %s could not be opened for reading!\n", filePath);
        return;
    }
    AccountInfo_s acc;
    while (fscanf(fptr, "%s %s %s %s %s %c\n", acc.username, acc.password, acc.email, acc.phone, acc.role, &acc.status) != EOF)
    {
        list = insertAtTail(list, acc);
    }

    fclose(fptr);
}

void addAccountToFile(char *filePath, AccountInfo_s acc)
{
    FILE *fptr = fopen(filePath, "a");
    if (fptr == NULL)
    {
        printf("File %s could not be opened for writing!\n", filePath);
        return;
    }

    fprintf(fptr, "%s %s %s %s %s %c\n", acc.username, acc.password, acc.email, acc.phone, acc.role, acc.status);

    fclose(fptr);
}

void updateAccountToFile(char *filePath, Llist_s *list)
{
    FILE *fptr = fopen(filePath, "w");
    if (fptr == NULL)
    {
        printf("File %s could not be opened for writing!\n", filePath);
        return;
    }
    Llist_s *pt;
    for (pt = list->next; pt != NULL; pt = pt->next)
    {
        fprintf(fptr, "%s %s %s %s %s %c\n", pt->nodeInfo.username, pt->nodeInfo.password, pt->nodeInfo.email, pt->nodeInfo.phone, pt->nodeInfo.role, pt->nodeInfo.status);
    }

    fclose(fptr);
}

AccountInfo_s searchAccountInFile(char *filePath, char *username)
{
    AccountInfo_s acc = {"", "", "", "", "", '0'};
    FILE *fptr = fopen(filePath, "r");
    if (fptr == NULL)
    {
        printf("File %s could not be opened for reading!\n", filePath);
        return acc;
    }
    char fileUsername[MAX_LEN];
    char filePassword[MAX_LEN];
    char fileEmail[MAX_LEN];
    char filePhone[MAX_LEN];
    char fileRole[MAX_LEN];
    char fileStatus;

    while (fscanf(fptr, "%s %s %s %s %s %c\n", fileUsername, filePassword, fileEmail, filePhone, fileRole, &fileStatus) != EOF)
    {
        if (strcmp(fileUsername, username) == 0)
        {
            strcpy(acc.username, fileUsername);
            strcpy(acc.password, filePassword);
            strcpy(acc.email, fileEmail);
            strcpy(acc.phone, filePhone);
            strcpy(acc.role, fileRole);
            acc.status = fileStatus;
            fclose(fptr);

            return acc;
        }
    }

    fclose(fptr);
    return acc;
}

void updateAccountStatusInFile(char *filePath, char *username, char newStatus)
{
    FILE *fptr = fopen(filePath, "r+");
    if (fptr == NULL)
    {
        printf("File %s could not be opened for updating!\n", filePath);
        return;
    }

    char fileUsername[MAX_LEN];
    char filePassword[MAX_LEN];
    char fileEmail[MAX_LEN];
    char filePhone[MAX_LEN];
    char fileRole[MAX_LEN];
    char fileStatus;

    long pos;
    while ((pos = ftell(fptr)) != -1 && fscanf(fptr, "%s %s %s %s %s %c\n", fileUsername, filePassword, fileEmail, filePhone, fileRole, &fileStatus) != EOF) // Đọc check cả \n
    {
        if (strcmp(fileUsername, username) == 0)
        {
            fseek(fptr, pos + strlen(fileUsername) + strlen(filePassword) + strlen(fileEmail) + strlen(filePhone) + strlen(fileRole) + 6, SEEK_SET);
            fputc(newStatus, fptr);
            break;
        }
    }

    fclose(fptr);
}

AccountInfo_s checkAccountStatusInFile(char *filePath, char *username, char *password, int *attempts)
{
    AccountInfo_s searchAcc = searchAccountInFile(filePath, username);
    AccountInfo_s resultAcc;

    if (strcmp(searchAcc.username, "") == 0)
    {
        printf("Cannot find account\n");
        strcpy(resultAcc.username, "");
        strcpy(resultAcc.password, "");
        strcpy(resultAcc.email, "");
        strcpy(resultAcc.phone, "");
        strcpy(resultAcc.role, "");
        resultAcc.status = '0';
        return resultAcc;
    }

    if (searchAcc.status == '0')
    {
        printf("Account is blocked\n");
        strcpy(resultAcc.username, "");
        strcpy(resultAcc.password, "");
        strcpy(resultAcc.email, "");
        strcpy(resultAcc.phone, "");
        strcpy(resultAcc.role, "");
        resultAcc.status = '0';
        return resultAcc;
    }

    if (strcmp(searchAcc.password, password) != 0)
    {
        printf("Incorrect password\n");
        (*attempts)++;

        if (*attempts >= 3)
        {
            printf("Account is blocked due to 3 failed login attempts\n");
            // Update the account status in the file to '0' (blocked)
            updateAccountStatusInFile(filePath, username, '0');
            // Return đúng tài khoản nhưng sai mật khẩu
            strcpy(resultAcc.username, searchAcc.username);
            strcpy(resultAcc.password, "");
            strcpy(resultAcc.email, "");
            strcpy(resultAcc.phone, "");
            strcpy(resultAcc.role, "");
            resultAcc.status = '0';

            return resultAcc;
        }

        printf("You have %d attempt(s) left\n", 3 - *attempts);
        strcpy(resultAcc.username, searchAcc.username);
        strcpy(resultAcc.password, "");
        strcpy(resultAcc.email, "");
        strcpy(resultAcc.phone, "");
        strcpy(resultAcc.role, "");
        resultAcc.status = '0';
        return resultAcc;
    }
    else
    {
        *attempts = 0; // Reset attempts on successful login
        return searchAcc;
    }
}

void addSignInHistory(char *filePath, AccountInfo_s acc)
{
    FILE *fptr = fopen(filePath, "a");
    if (fptr == NULL)
    {
        printf("File %s could not be opened for writing!\n", filePath);
        return;
    }
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char dateStr[11];
    char timeStr[20];

    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", t);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", t);

    fprintf(fptr, "%s | %s | %s\n", acc.username, dateStr, timeStr);

    fclose(fptr);
}

void accountRegister(Llist_s *list)
{
    AccountInfo_s newAcc;
    AccountInfo_s searchAcc;

    do
    {
        // Nhập username - tự động loại bỏ tất cả dấu cách
        do
        {
            getInputNoSpaces("Username: ", newAcc.username, MAX_LEN);
            if (strlen(newAcc.username) == 0)
                printf("Username cannot be empty. Please try again.\n");
        } while (strlen(newAcc.username) == 0);

        // Nhập password - tự động loại bỏ tất cả dấu cách
        do
        {
            getInputNoSpaces("Password: ", newAcc.password, MAX_LEN);
            if (strlen(newAcc.password) == 0)
                printf("Password cannot be empty. Please try again.\n");
        } while (strlen(newAcc.password) == 0);

        // Nhập email - tự động loại bỏ tất cả dấu cách và validation
        do
        {
            getInputAutoRemoveSpaces("Email: ", newAcc.email, MAX_LEN);
            // if (strlen(newAcc.email) == 0)
            // {
            //     printf("Email cannot be empty. Please try again.\n");
            //     continue;
            // }
            // if (!checkEmail(newAcc.email))
            //     printf("Invalid email format. Please try again.\n");
        } while (strlen(newAcc.email) == 0 || !checkEmail(newAcc.email));

        // Nhập phone - tự động loại bỏ tất cả dấu cách và validation
        do
        {
            getInputAutoRemoveSpaces("Phone: ", newAcc.phone, MAX_LEN);
            // if (strlen(newAcc.phone) == 0)
            // {
            //     printf("Phone number cannot be empty. Please try again.\n");
            //     continue;
            // }
            // if (!checkPhoneNumber(newAcc.phone))
            //     printf("Invalid phone number format (must be 10 digits starting with 0). Please try again.\n");
        } while (strlen(newAcc.phone) == 0 || !checkPhoneNumber(newAcc.phone));

        // Nhập role với xử lý dấu cách (cho phép dấu cách ở giữa)
        do
        {
            getInputWithTrim("Role: ", newAcc.role, MAX_LEN);
            if (strlen(newAcc.role) == 0)
                printf("Role cannot be empty. Please try again.\n");
        } while (strlen(newAcc.role) == 0);

        newAcc.status = '1';

        searchAcc = searchAccountInFile(INPUT_FILE_PATH, newAcc.username);

        if (strcmp(searchAcc.username, "") != 0)
            printf("Username already exists. Please choose a different username.\n");

    } while (strcmp(searchAcc.username, "") != 0);

    list = insertAtTail(list, newAcc);
    addAccountToFile(INPUT_FILE_PATH, newAcc);
    printf("Successful registration\n");
}

void accountSignIn(Llist_s *list)
{
    char username[MAX_LEN];
    char password[MAX_LEN];
    int attempts = 0;

    AccountInfo_s validAcc;

    // Nhập username - tự động loại bỏ tất cả dấu cách
    getInputNoSpaces("Username: ", username, MAX_LEN);

    /*
    Tiếp tục nếu
        TK username đúng nhưng sai mật khẩu và attempts < 3
    Break nếu
        TK username đúng và mật khẩu đúng
    */
    do
    {
        // Nhập password - tự động loại bỏ tất cả dấu cách
        getInputNoSpaces("Password: ", password, MAX_LEN);

        validAcc = checkAccountStatusInFile(INPUT_FILE_PATH, username, password, &attempts);

    } while (attempts < 3 && strcmp(validAcc.username, username) == 0 && strcmp(validAcc.password, "") == 0);

    if (strcmp(validAcc.username, "") == 0 || strcmp(validAcc.password, "") == 0)
        return;

    if (strcmp(validAcc.role, "admin") == 0)
    {
        adminMode = 1;
        printf("Admin mode activated.\n");
    }

    printf("Welcome %s\n", validAcc.username);

    // Update phiên đăng nhập
    isLoggedIn = 1;
    loggedInUser = validAcc;

    // Update lịch sử đăng nhập
    addSignInHistory(HISTORY_FILE_PATH, validAcc);
}

void accountSignOut()
{
    if (!isLoggedIn)
    {
        printf("No user is currently logged in.\n");
        return;
    }

    printf("User %s signed out successfully.\n", loggedInUser.username);
    isLoggedIn = 0;
    memset(&loggedInUser, 0, sizeof(loggedInUser));
}

void changePassword(Llist_s *list)
{
    if (!isLoggedIn)
    {
        printf("No user is currently logged in. Please sign in first.\n");
        return;
    }

    char currentPassword[MAX_LEN];
    char newPassword[MAX_LEN];
    char confirmPassword[MAX_LEN];

    // Nhập mật khẩu hiện tại - tự động loại bỏ tất cả dấu cách
    getInputNoSpaces("Enter current password: ", currentPassword, MAX_LEN);

    if (strcmp(loggedInUser.password, currentPassword) != 0)
    {
        printf("Current password is incorrect.\n");
        return;
    }

    // Nhập mật khẩu mới - tự động loại bỏ tất cả dấu cách
    getInputNoSpaces("Enter new password: ", newPassword, MAX_LEN);

    // Cập nhật mật khẩu trong danh sách liên kết
    Llist_s *pt;
    pt = findNodeByUsername(list, loggedInUser.username);
    if (pt != NULL)
    {
        strcpy(pt->nodeInfo.password, newPassword);
        loggedInUser = pt->nodeInfo; // Cập nhật thông tin người dùng đã đăng nhập
    }
    else
    {
        printf("Error: Logged in user not found in the list.\n");
        return;
    }

    // Cập nhật mật khẩu trong file
    updateAccountToFile(INPUT_FILE_PATH, list);

    printf("Password changed successfully.\n");
}

void updateAccountInfo(Llist_s *list)
{
    if (!isLoggedIn)
    {
        printf("No user is currently logged in. Please sign in first.\n");
        return;
    }

    char newEmail[MAX_LEN];
    char newPhone[MAX_LEN];

    printf("========= User Information Update =========\n");
    printf("1. Update Email\n");
    printf("2. Update Phone Number\n");
    printf("Your choice (1-2, other to back): ");

    int choice;
    if (scanf("%d", &choice) != 1 || choice < 1 || choice > 2)
        return;

    switch (choice)
    {
    case 1:
        // Cập nhật email - tự động loại bỏ tất cả dấu cách và validation
        do
        {
            getInputAutoRemoveSpaces("Enter new email: ", newEmail, MAX_LEN);
            // if (strlen(newEmail) == 0)
            // {
            //     printf("Email cannot be empty. Please try again.\n");
            //     continue;
            // }
            // if (!checkEmail(newEmail))
            //     printf("Invalid email format. Please try again.\n");
        } while (strlen(newEmail) == 0 || !checkEmail(newEmail));
        break;
        
    case 2:
        // Cập nhật phone - tự động loại bỏ tất cả dấu cách và validation
        do
        {
            getInputAutoRemoveSpaces("Enter new phone: ", newPhone, MAX_LEN);
            // if (strlen(newPhone) == 0)
            // {
            //     printf("Phone number cannot be empty. Please try again.\n");
            //     continue;
            // }
            // if (!checkPhoneNumber(newPhone))
            //     printf("Invalid phone number format (must be 10 digits starting with 0). Please try again.\n");
        } while (strlen(newPhone) == 0 || !checkPhoneNumber(newPhone));
        break;
        
    default:
        return;
    }

    // Cập nhật thông tin trong danh sách liên kết
    Llist_s *pt;
    pt = findNodeByUsername(list, loggedInUser.username);
    if (pt != NULL)
    {
        if (choice == 1)
            strcpy(pt->nodeInfo.email, newEmail);
        else if (choice == 2)
            strcpy(pt->nodeInfo.phone, newPhone);
        loggedInUser = pt->nodeInfo; // Cập nhật thông tin người dùng đã đăng nhập
    }
    else
    {
        printf("Error: Logged in user not found in the list.\n");
        return;
    }

    // Cập nhật thông tin trong file
    updateAccountToFile(INPUT_FILE_PATH, list);

    printf("Account information updated successfully.\n");
}

void resetPassword(Llist_s *list)
{
    if (!isLoggedIn)
    {
        printf("No user is currently logged in. Please sign in first.\n");
        return;
    }

    char newPassword[MAX_LEN];
    char *otpCode = "123456"; // Mã OTP tạm thời

    printf("An OTP code has been sent to your registered email/phone. (For demo, OTP is %s)\n", otpCode);
    char enteredOtp[MAX_LEN];

    // Nhập OTP - tự động loại bỏ tất cả dấu cách
    getInputNoSpaces("Enter OTP code: ", enteredOtp, MAX_LEN);

    if (strcmp(enteredOtp, otpCode) != 0)
    {
        printf("Invalid OTP code. Password reset failed.\n");
        return;
    }

    // Nhập mật khẩu mới - tự động loại bỏ tất cả dấu cách
    getInputNoSpaces("Enter new password: ", newPassword, MAX_LEN);

    // Cập nhật mật khẩu trong danh sách liên kết
    Llist_s *pt;
    pt = findNodeByUsername(list, loggedInUser.username);
    if (pt != NULL)
    {
        strcpy(pt->nodeInfo.password, newPassword);
        loggedInUser = pt->nodeInfo; // Cập nhật thông tin người dùng đã đăng nhập
    }
    else
    {
        printf("Error: Logged in user not found in the list.\n");
        return;
    }

    // Cập nhật mật khẩu trong file
    updateAccountToFile(INPUT_FILE_PATH, list);

    printf("Password reset successfully.\n");
}

void viewLoginHistory()
{
    if (!isLoggedIn)
    {
        printf("No user is currently logged in. Please sign in first.\n");
        return;
    }

    FILE *fptr = fopen(HISTORY_FILE_PATH, "r");
    if (fptr == NULL)
    {
        printf("File %s could not be opened for reading!\n", HISTORY_FILE_PATH);
        return;
    }

    char line[100];
    printf("========= Login History for %s =========\n", loggedInUser.username);
    while (fgets(line, sizeof(line), fptr) != NULL)
    {
        if (strstr(line, loggedInUser.username) != NULL)
        {
            printf("%s", line);
        }
    }

    fclose(fptr);
}

/*
=====================ADMIN FUNCTIONS=====================
*/

void viewAllAccounts(Llist_s *list)
{
    if (!adminMode)
    {
        printf("Admin mode is not activated.\n");
        return;
    }

    printf("============ All Accounts ============\n");
    Llist_s *pt;
    for (pt = list->next; pt != NULL; pt = pt->next)
    {
        printf("Username: %s, Email: %s, Phone: %s, Role: %s, Status: %c\n",
               pt->nodeInfo.username,
               pt->nodeInfo.email,
               pt->nodeInfo.phone,
               pt->nodeInfo.role,
               pt->nodeInfo.status);
    }

    return;
}

void deleteAccount(Llist_s *list)
{
    if (!adminMode)
    {
        printf("Admin mode is not activated.\n");
        return;
    }

    viewAllAccounts(list);
    printf("=====================================\n");

    char username[MAX_LEN];
    printf("Enter the username to delete: ");
    scanf("%s", username);
    clearInputBuffer();

    if (strcmp(username, loggedInUser.username) == 0)
    {
        printf("You cannot delete your own account while logged in.\n");
        return;
    }

    Llist_s *pt = findNodeByUsername(list, username);
    if (pt == NULL)
    {
        printf("Account with username %s not found.\n", username);
        return;
    }

    // Xóa tài khoản khỏi Linked list và cập nhật file
    deleteNodeByUsername(list, username);
    updateAccountToFile(INPUT_FILE_PATH, list);

    printf("Account with username %s has been deleted successfully.\n", username);
}

void resetPasswordForAccounts(Llist_s *list)
{
    if (!adminMode)
    {
        printf("Admin mode is not activated.\n");
        return;
    }

    viewAllAccounts(list);
    printf("=====================================\n");

    char username[MAX_LEN];
    printf("Enter the username to reset password: ");
    scanf("%s", username);
    clearInputBuffer();

    if (strcmp(username, loggedInUser.username) == 0)
    {
        printf("Be careful. Changing ADMIN password ?\n");
        printf("1. Confirm\n");
        printf("2. Cancel\n");
        int choice;
        if (scanf("%d", &choice) != 1 || choice != 1)
            return;
    }

    Llist_s *pt = findNodeByUsername(list, username);
    if (pt == NULL)
    {
        printf("Account with username %s not found.\n", username);
        return;
    }

    char newPassword[MAX_LEN];
    printf("Enter new password for %s: ", username);
    scanf("%s", newPassword);
    clearInputBuffer();

    // Cập nhật mật khẩu trong danh sách liên kết
    strcpy(pt->nodeInfo.password, newPassword);

    // Cập nhật mật khẩu trong file
    updateAccountToFile(INPUT_FILE_PATH, list);

    printf("Password for account with username %s has been reset successfully.\n", username);
}

void adminPanel(Llist_s *list)
{
    if (!adminMode)
    {
        printf("Admin mode is not activated.\n");
        return;
    }

    while (1)
    {
        adminMenu();
        int choice;
        if (scanf("%d", &choice) != 1 || choice < 1 || choice > 3)
        {
            printf("Exiting Admin mode.\n");
            adminMode = 0;
            isLoggedIn = 0;
            memset(&loggedInUser, 0, sizeof(loggedInUser));
            break;
        }
        switch (choice)
        {
        case 1:
            viewAllAccounts(list);
            break;
        case 2:
            deleteAccount(list);
            break;
        case 3:
            resetPasswordForAccounts(list);
            break;
        default:
            break;
        }
    }

    return;
}

int main()
{
    Llist_s *accountList = newList();
    readAccountsFromFile(INPUT_FILE_PATH, accountList);

    while (1)
    {
        menu();
        int choice;
        if (scanf("%d", &choice) != 1 || choice < 1 || choice > 7)
        {
            printf("Exiting program.\n");
            exit(0);
        }
        clearInputBuffer();
        switch (choice)
        {
        case 1:
            accountRegister(accountList);
            break;
        case 2:
            accountSignIn(accountList);
            if (adminMode)
                adminPanel(accountList);
            break;
        case 3:
            // changePassword(&accountList);
            changePassword(accountList);
            break;
        case 4:
            // updateAccountInfo(&accountList);
            updateAccountInfo(accountList);
            break;
        case 5:
            // resetProgram(&accountList);
            resetPassword(accountList);
            break;
        case 6:
            // viewLoginHistory(&accountList);
            viewLoginHistory();
            break;
        case 7:
            // signOut(&accountList);
            accountSignOut();
            break;
        default:
            break;
        }
    }
}