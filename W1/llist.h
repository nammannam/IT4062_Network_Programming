#include <string.h>
#include <stdio.h>

#define MAX_LEN 20

typedef struct AccountInfo{
    char username[MAX_LEN];
    char password[MAX_LEN];
    char email[MAX_LEN];
    char phone[MAX_LEN];
    char role[MAX_LEN];
    char status;

}AccountInfo_s;

typedef struct Llist{
    AccountInfo_s nodeInfo;
    struct Llist *next;

}Llist_s;

Llist_s* newList(){
    Llist_s* list = (Llist_s *)malloc(sizeof(Llist_s));
    list->next = NULL;
    
    return list;

}

Llist_s* makeNewNode(AccountInfo_s info){
    Llist_s * newNode = (Llist_s *)malloc(sizeof(Llist_s));
    newNode->nodeInfo = info;
    newNode->next = NULL;
    return newNode;
    
}

Llist_s* insertAtHead(Llist_s *list, AccountInfo_s info){
    Llist_s * newNode = makeNewNode(info);
    newNode->next = list->next;
    list->next = newNode;
    return list;
    
}

Llist_s* insertAtTail(Llist_s *list, AccountInfo_s info){
    Llist_s * newNode = makeNewNode(info);
    Llist_s *pt;
    for(pt = list ; pt->next != NULL ; pt = pt->next);
        pt->next = newNode;
    return list;
    
}

/*
Search for a user by username in the linked list
Input: list - pointer to the head of the linked list
       username - the username to search for

Output: the AccountInfo_s structure of the found user, or an empty structure if not found

*/
AccountInfo_s searchUsernameList(Llist_s *list, char username[]){

    Llist_s *pt;
    for(pt = list ; pt != NULL ; pt = pt->next){
        if(strcasecmp(pt->nodeInfo.username, username) == 0){

            return pt->nodeInfo;
        }
    }
    return (AccountInfo_s){"", "", "", "", "", '0'};
    
}

/*
Check if the provided password matches the account's password
Input: llist - pointer to the head of the linked list
       account - the AccountInfo_s structure of the user
       password - the password to check

Output: the AccountInfo_s structure if the password matches, or an empty structure if not
*/
AccountInfo_s checkAccountPassword(Llist_s *llist ,AccountInfo_s account, char password[]){
    Llist_s *pt;
    for(pt = llist ; pt != NULL ; pt = pt->next){
        if(strcasecmp(pt->nodeInfo.username, account.username) == 0 && strcasecmp(pt->nodeInfo.password, password) == 0){
            return pt->nodeInfo;
        }
    }
    return (AccountInfo_s){"", "", "", "", "", '0'};
}

Llist_s* findNodeByUsername(Llist_s *list, char username[]){
    Llist_s *pt;
    for(pt = list ; pt != NULL ; pt = pt->next){
        if(strcasecmp(pt->nodeInfo.username, username) == 0){

            return pt;
        }
    }
    return NULL;
    

}


void deleteNodeByUsername(Llist_s *list, char username[]){
    Llist_s *pt;
    for(pt = list ; pt->next != NULL ; pt = pt->next){
        if(strcasecmp(pt->next->nodeInfo.username, username) == 0){
            Llist_s *temp = pt->next;
            pt->next = pt->next->next;
            free(temp);
            return;
        }
    }
    
}


void deleteList(Llist_s *list){
    Llist_s *pt = list;
    while(pt != NULL){
        Llist_s *temp = pt;
        pt = pt->next;
        free(temp);
    }
    
}







