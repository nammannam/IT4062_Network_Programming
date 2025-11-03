#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include "llist.h"
#include "protocol.h"


#define INPUT_FILE_PATH "./account.txt"
#define HISTORY_FILE_PATH "./history.txt"
#define MAX_LEN 20
#define BUFFER_SIZE 1024

AccountInfo_s loggedInUser; // Current logged-in user
int isLoggedIn = 0;         // 0 - no user logged in, 1 - user logged in

void readAccountsFromFile(char *filePath, Llist_s *list)
{
    FILE *fptr = fopen(filePath, "r");
    if (fptr == NULL)
    {
        printf("File %s could not be opened for reading!\n", filePath);
        exit(EXIT_FAILURE);
    }
    AccountInfo_s acc;
    while (fscanf(fptr, "%s %s %s %s %c\n", acc.username, acc.password, acc.email, acc.homepage, &acc.status) != EOF)
    {
        list = insertAtTail(list, acc);
    }

    fclose(fptr);
}

void updateAccountListFromFile(char *filePath, Llist_s *list)
{
    FILE *fptr = fopen(filePath, "r");
    if (fptr == NULL)
    {
        printf("File %s could not be opened for reading!\n", filePath);
        exit(EXIT_FAILURE);
    }
    AccountInfo_s acc;
    list->next = NULL; // Clear existing list
    while (fscanf(fptr, "%s %s %s %s %c\n", acc.username, acc.password, acc.email, acc.homepage, &acc.status) != EOF)
    {
        list = insertAtTail(list, acc);
    }

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
        fprintf(fptr, "%s %s %s %s %c\n", pt->nodeInfo.username, pt->nodeInfo.password, pt->nodeInfo.email, pt->nodeInfo.homepage, pt->nodeInfo.status);
    }

    fclose(fptr);
}

AccountInfo_s searchAccountInFile(char *filePath, char *username)
{
    AccountInfo_s acc = {"", "", "", "", '0'};
    FILE *fptr = fopen(filePath, "r");
    if (fptr == NULL)
    {
        printf("File %s could not be opened for reading!\n", filePath);
        return acc;
    }
    char fileUsername[MAX_LEN];
    char filePassword[MAX_LEN];
    char fileEmail[MAX_LEN];
    char fileHomepage[MAX_LEN];
    char fileStatus;

    while (fscanf(fptr, "%s %s %s %s %c\n", fileUsername, filePassword, fileEmail, fileHomepage, &fileStatus) != EOF)
    {
        if (strcmp(fileUsername, username) == 0)
        {
            strcpy(acc.username, fileUsername);
            strcpy(acc.password, filePassword);
            strcpy(acc.email, fileEmail);
            strcpy(acc.homepage, fileHomepage);
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
    char fileHomepage[MAX_LEN];
    char fileStatus;

    long pos;
    while ((pos = ftell(fptr)) != -1 && fscanf(fptr, "%s %s %s %s %c\n", fileUsername, filePassword, fileEmail, fileHomepage, &fileStatus) != EOF) // Đọc check cả \n
    {
        if (strcmp(fileUsername, username) == 0)
        {
            fseek(fptr, pos + strlen(fileUsername) + strlen(filePassword) + strlen(fileEmail) + strlen(fileHomepage) + 4, SEEK_SET);
            fputc(newStatus, fptr);
            break;
        }
    }

    fclose(fptr);
}

int accountSignIn(Llist_s *list, char *filePath, char *username, char *password, int *attempts)
{
    AccountInfo_s searchAcc = searchUsernameList(list, username);
    AccountInfo_s resultAcc;

    //Không tìm thấy Account -> Trả về struct rỗng -> 0
    if (strcmp(searchAcc.username, "") == 0)
    {
        printf("Cannot find account\n");
        return 0;
    }

    //Account bị khóa -> Trả về chỉ status = '0' -> 1
    if (searchAcc.status == '0')
    {
        printf("Account is blocked\n");
        return 1;
    }

    if (strcmp(searchAcc.password, password) != 0)
    {
        printf("Incorrect password\n");
        (*attempts)++;

        //Khóa account sau 3 lần nhập sai password -> trả về username và status = 0 -> 2
        if (*attempts >= 3)
        {
            printf("Account is blocked due to 3 failed login attempts\n");
            // Update the account status in the file to '0' (blocked)
            updateAccountStatusInFile(filePath, username, '0');
            updateAccountListFromFile(filePath, list); //Cập nhật lại danh sách liên kết
            
            return 2;
        }

        //Sai mật khẩu Account nhưng còn attempts -> Trả về chỉ username -> 3
        printf("You have %d attempt(s) left\n", 3 - *attempts);

        return 3;
    }
    else
    {
        // Đăng nhập thành công -> Trả về đầy đủ thông tin Account đã đăng nhập -> 4
        *attempts = 0; // Reset attempts on successful login
        loggedInUser = searchAcc; // Cập nhật thông tin người dùng đã đăng nhập
        isLoggedIn = 1;           // Cập nhật trạng thái đã đăng nhập
        return 4;
    }
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




char *stringProcess(char *str){

    char *temp1 = malloc(strlen(str) + 1);
    char *temp2 = malloc(strlen(str) + 1);
    char *res = malloc(strlen(str) + 1);
    memset(temp1, 0, strlen(str) + 1);
    memset(temp2, 0, strlen(str) + 1);
    memset(res, 0, strlen(str) + 1);

    for(int i = 0 ; i < strlen(str) ; i++){
        if(str[i] >= '0' && str[i] <= '9'){
            strncat(temp1, &str[i], 1);
        }
        if((str[i] >= 'a' && str[i] <= 'z') ||( str[i] >= 'A' && str[i] <= 'Z')){
            strncat(temp2, &str[i], 1);
        }

    }
    if(strlen(temp1) == 0){
        sprintf(res, "\n%s\n", temp2);
        free(temp1);
        free(temp2);
        
    }else if(strlen(temp2) == 0){
        sprintf(res, "\n%s\n", temp1);
        free(temp1);
        free(temp2);
    }else{
        sprintf(res, "\n%s\n%s\n", temp1, temp2);
        free(temp1);
        free(temp2);
    }

    return res;
}

int stringCheck(char *str){

    int flag = 1;

    for(int i = 0 ; i < strlen(str) ; i++){

        if(!(str[i] >= 'a' && str[i] <= 'z') && !( str[i] >= 'A' && str[i] <= 'Z') && !(str[i] >= '0' && str[i] <= '9')){
            flag = 0;
            break;
        }

    }

    if(flag){
        return 1;
    }else{
        return 0;
    }
}

void changePassword(Llist_s *list, char *newPassword, char *reply)
{
    char *replyResult;

    // // Nhập mật khẩu mới - tự động loại bỏ tất cả dấu cách
    // getInputNoSpaces("Enter new password: ", newPassword, MAX_LEN);

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

    if(stringCheck(newPassword) == 0){
        sprintf(reply, "Error\n");
        return;
    }
    // Cập nhật mật khẩu trong file
    updateAccountToFile(INPUT_FILE_PATH, list);

    replyResult = stringProcess(newPassword);

    sprintf(reply, "%s", replyResult);
    free(replyResult);
    printf("Password changed successfully.\n");
}

int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: %s <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char username[MAX_LEN];
    char password[MAX_LEN];
    int attempts = 0;
    memset(username, 0, sizeof(username));
    memset(password, 0, sizeof(password));

    //Lấy server port
    char *endptr;
    long servPort_l = strtol(argv[1], &endptr, 10);
    if(*endptr != '\0' || servPort_l <= 0 || servPort_l > 65535){
        fprintf(stderr, "Invalid port: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    uint16_t servPort = (uint16_t)servPort_l;
    
    
    Llist_s *list = newList();
    readAccountsFromFile(INPUT_FILE_PATH, list);

    // int socketfd;
    int listenfd;
    int connfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in servaddr, cliaddr;
    socklen_t cliaddr_len = sizeof(cliaddr);

    // Tạo socket TCP
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            perror("socket creation failed");
            exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0 , sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));


    //Cấu hình Server UDP
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(servPort);

    //Gán IP và Port cho Socket
    if(bind(listenfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        perror("bind failed");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    //Chuyển sang trạng thái lắng nghe kết nối từ Client (max 5 kết nối chờ)
    if(listen(listenfd, 5) < 0){
        perror("listen failed");
        close(listenfd);
        exit(EXIT_FAILURE);
    }else{
        printf("Server is listening on port %d...\n", servPort);
    }

    // Khai báo biến cho message
    application_msg_t msgIn;    // Nhận message từ Client
    application_msg_t msgOut;   // Gửi message đến Client
    memset(&msgOut, 0, sizeof(msgOut));
    memset(&msgIn, 0, sizeof(msgIn));

    for( ; ; ){

        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
        if (connfd < 0) {
            perror("accept failed");
            continue;
        }else{
            printf("Accepted connection from client (%s:%d)\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

        }
        
        while(1){
        
            int rcvBytes = recv(connfd, &msgIn, sizeof(msgIn), 0);

        if(rcvBytes < 0){
            perror("Error: ");
            return 0;
        }else if(rcvBytes == 0){
            printf("Client (%s:%d) disconnected unexpectedly.\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
            
            accountSignOut();
            memset(username, 0, sizeof(username)); //Reset username để nhận lại từ Client
            memset(password, 0, sizeof(password)); //Reset password để nhận lại từ Client
            attempts = 0; //Reset attempts để nhận lại từ Client
            isLoggedIn = 0; //Reset trạng thái đăng nhập để đăng nhập lại
            break;

        }

        server_handle_message(&msgIn, &msgOut, list);

        // buffer[rcvBytes] = '\0';

        // char reply[BUFFER_SIZE];
        // memset(reply, 0, sizeof(reply));
        
        // printf("Received from client (%s:%d): %s\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), buffer);

        
        //Xử lý thông tin nhận từ Client
        if(strcmp(msgIn.value, "bye") == 0 && isLoggedIn == 1){
            //
            printf("Client (%s:%d) disconnected.\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
            accountSignOut();
            memset(username, 0, sizeof(username)); //Reset username để nhận lại từ Client
            memset(password, 0, sizeof(password)); //Reset password để nhận lại từ Client
            attempts = 0; //Reset attempts để nhận lại từ Client
            isLoggedIn = 0; //Reset trạng thái đăng nhập để đăng nhập lại

            // strcpy(reply, "Goodbye HUST\n");

            create_message(&msgOut, MSG_CF, "Goodbye HUST\n");

            // sendto(socketfd, reply, strlen(reply), 0, (const struct sockaddr *)&cliaddr, cliaddr_len);
            send(connfd, &msgOut, sizeof(msgOut), 0);

        }else if(isLoggedIn == 0){
            // printf("CHECKKKKK\n");

            char bufferUsername[MAX_LEN];

            strcpy(bufferUsername, msgIn.value);

            // Xử lý thông tin nhận từ Client
            AccountInfo_s accountInfo = searchAccountInFile(INPUT_FILE_PATH, bufferUsername);
            
            // Phần này chưa sử dụng server_handle_message vì phải xử lý đăng nhập tài khoản có trên file .txt
            if(strcmp(accountInfo.username, "") == 0){
                // strcpy(reply, "Cannot find account\n");

                create_message(&msgOut, MSG_DENY, "Cannot find account\n");

                // sendto(socketfd, reply, strlen(reply), 0, (const struct sockaddr *)&cliaddr, cliaddr_len);
                send(connfd, &msgOut, sizeof(msgOut), 0);
                memset(username, 0, sizeof(username)); //Reset username để nhận lại từ Client
                memset(password, 0, sizeof(password)); //Reset password để nhận lại từ Client
                attempts = 0; //Reset attempts để nhận lại từ Client
                isLoggedIn = 0; //Reset trạng thái đăng nhập để đăng nhập lại
            
            }else{ 
                // strcpy(reply, "OK\n");

                create_message(&msgOut, MSG_CF, accountInfo.username);
                loggedInUser = accountInfo; // Lưu thông tin tài khoản tìm được
                isLoggedIn = 1; // Cập nhật trạng thái đã đăng nhập

                // sendto(socketfd, reply, strlen(reply), 0, (const struct sockaddr *)&cliaddr, cliaddr_len);
                send(connfd, &msgOut, sizeof(msgOut), 0);

            }

            /*
            if(username[0] == '\0'){
                //Nhận username từ Client
                strcpy(username, buffer);
                printf("Username received: %s\n", username);

                strcpy(reply, "Insert password:\n");
                // sendto(socketfd, reply, strlen(reply), 0, (const struct sockaddr *)&cliaddr, cliaddr_len);
                send(connfd, reply, strlen(reply), 0);

            }else if(password[0] == '\0' && username[0] != '\0' && isLoggedIn == 0){
                //Nhận password từ Client
                strcpy(password, buffer);
                printf("Password received: %s\n", password);

                int returnSignInCode = accountSignIn(list, INPUT_FILE_PATH, username, password, &attempts);

                if(returnSignInCode == 0){
                    strcpy(reply, "Cannot find account\n");
                    // sendto(socketfd, reply, strlen(reply), 0, (const struct sockaddr *)&cliaddr, cliaddr_len);
                    send(connfd, reply, strlen(reply), 0);
                    memset(username, 0, sizeof(username)); //Reset username để nhận lại từ Client
                    memset(password, 0, sizeof(password)); //Reset password để nhận lại từ Client
                    attempts = 0; //Reset attempts để nhận lại từ Client
                    

                }else if(returnSignInCode == 1){//Tài khoản bị khóa từ trước
                    strcpy(reply, "Account was blocked\n");
                    // sendto(socketfd, reply, strlen(reply), 0, (const struct sockaddr *)&cliaddr, cliaddr_len);
                    send(connfd, reply, strlen(reply), 0);
                    memset(username, 0, sizeof(username)); //Reset username để nhận lại từ Client
                    memset(password, 0, sizeof(password)); //Reset password để nhận lại từ Client
                    attempts = 0; //Reset attempts để nhận lại từ Client
                    

                }else if(returnSignInCode == 2){//Tài khoản bị khóa do hết attempts
                    strcpy(reply, "Account is blocked\n");
                    // sendto(socketfd, reply, strlen(reply), 0, (const struct sockaddr *)&cliaddr, cliaddr_len);
                    send(connfd, reply, strlen(reply), 0);
                    memset(username, 0, sizeof(username)); //Reset username để nhận lại từ Client
                    memset(password, 0, sizeof(password)); //Reset password để nhận lại từ Client
                    attempts = 0; //Reset attempts để nhận lại từ Client
                    

                }else if(returnSignInCode == 3){//Sai mật khẩu nhưng còn attempts
                   strcpy(reply, "Not OK\n");
                //    sendto(socketfd, reply, strlen(reply), 0, (const struct sockaddr *)&cliaddr, cliaddr_len);
                   send(connfd, reply, strlen(reply), 0);
                   memset(password, 0, sizeof(password)); //Reset password để nhận lại từ Client
                  

                }else if(returnSignInCode == 4){//Đăng nhập thành công
                    strcpy(reply, "OK\n");
                    // sendto(socketfd, reply, strlen(reply), 0, (const struct sockaddr *)&cliaddr, cliaddr_len);
                    send(connfd, reply, strlen(reply), 0);
                }

            }
            */

        }else if(isLoggedIn == 1){



            //Xử lý các lệnh từ Client khi đã đăng nhập - Xử lý Client gửi TEXT
            // server_handle_message(&msgIn, &msgOut);
            send(connfd, &msgOut, sizeof(msgOut), 0);
            


            /*
            if(strcmp(buffer, "homepage") != 0){
            //Nhận newPassword từ Client
            changePassword(list, buffer, reply);

            // sendto(socketfd, reply, strlen(reply), 0, (const struct sockaddr *)&cliaddr, cliaddr_len);
            send(connfd, reply, strlen(reply), 0);

            }else if(strcmp(buffer, "homepage") == 0){
                //Gửi homepage cho Client
                sprintf(reply, "%s\n", loggedInUser.homepage);
                // sendto(socketfd, reply, strlen(reply), 0, (const struct sockaddr *)&cliaddr, cliaddr_len);
                send(connfd, reply, strlen(reply), 0);

            }else{
               printf("Unknown command\n");

            }
            */
                
            
        }
        
    }
        close(connfd);
    }

    //
    close(listenfd);
    return 0;
}