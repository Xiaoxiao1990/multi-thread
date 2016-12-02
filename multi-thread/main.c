/**************
 *
 * Date : 2016-12-2
 * Author : ZLL
 *
 * VSIM client managerment.
 * **********/

#include <stdio.h>
#include <stdlib.h>             //exit()
#include <string.h>             //strerror()
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>          //ntoa()
#include <unistd.h>             //close()
#include <malloc.h>

#define VSIM_NAME_LEN  16

/**
 * @brief The Client_Info struct
 * A forward direction loop link node definition.
 */
typedef struct S_Client_Info{
    uint16_t num;
    in_addr_t ip;
    in_port_t port;
    int conn_fd;
    uint8_t device_name[VSIM_NAME_LEN];
    uint16_t time_out;                      //unit second.
    struct S_Client_Info *prev,*next;
}Client_TypeDef;

Client_TypeDef *current_client;

/**
 * @brief client_data_init
 * initialize the data node
 * @param ct struct *
 */
void client_data_init(Client_TypeDef *ct)
{
    uint16_t i;
    //client data link
    ct->num = 0;
    ct->conn_fd = -1;
    ct->ip = 0;
    ct->port = 0;
    ct->time_out = -1;
    for(i = 0;i < sizeof(ct->device_name);i++)
        ct->device_name[i] = '\0';
    ct->next = NULL;
    ct->prev = NULL;
}

/**
 * @brief add_new_client
 * add client to client data link
 * @param ct
 */
void add_new_client(Client_TypeDef *ct)
{
    ct->prev = current_client;
    ct->next = current_client->next;
    current_client->next->prev = ct;
    current_client->next = ct;

    current_client = ct;
}

/**
 * @brief delete_client_by_num
 * delet client by client number
 * @param num
 * client number
 */
void delete_client_by_num(uint16_t num)
{
    Client_TypeDef *dct = current_client;

    while(dct->num != num){
        if(dct->num > 0)dct = dct->prev;
        else {
            printf("Client [%d] not found !\n",num);
        }
    }
    //Delete this node dct.
    dct->prev->next = dct->next;
    dct->next->prev = dct->prev;
    free(dct);
}

/**
 * @brief delete_client_by_pos
 * delete client by structure pointer
 * @param ct
 * data structure pointer
 */
void delete_client_by_pos(Client_TypeDef *ct)
{
    Client_TypeDef *dct = current_client;

    while(dct != ct){
        dct = dct->prev;
        if(dct == current_client){
            printf("Client [%d] not found !\n",ct->num);
        }
    }
    //Delete this node dct.
    dct->prev->next = dct->next;
    dct->next->prev = dct->prev;
    free(dct);
}

#define VSIM_SOCKET_BUF_LEN     1024

/**
 * @brief client_stuff
 * this is multi-thread subroutine.
 * @param ct
 */
void client_stuff(Client_TypeDef *ct)
{
    char buff[VSIM_SOCKET_BUF_LEN];
    int n;

    while(ct->time_out > 0){
        char *from = inet_ntoa(ct->ip);
        printf("[%s:%d] Connected !\n", from, client.sin_port);
        n = recv(ct->conn_fd, buff, VSIM_SOCKET_BUF_LEN, 0);
        buff[n] = '\0';

        printf("Recv msg from client[%s:%d]:%s\n", from, client.sin_port, buff);
        close(connfd);
    }

}

#define SERVER_PORT             6666
#define MAX_LINKS               720
/**
 * @brief VSIM_Managerment
 * manager the clients
 */
void VSIM_Managerment(void)
{
    int listenfd, connfd;
    uint16_t client_num = 0;
    struct sockaddr_in servaddr;
//    char buff[VSIM_SOCKET_BUF_LEN];
//    int n;

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        printf("Create socket server error:%s(errorno:%d)\n",strerror(errno),errno);
        exit(0);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    if(bind(listenfd, (struct sockaddr *)&servaddr,sizeof(servaddr)) == -1){
        printf("Bind socket server error:%s(errorno:%d)\n",strerror(errno),errno);
        exit(0);
    }

    if(listen(listenfd, MAX_LINKS) == -1){
        printf("Listen socket error:%s(errorno:%d)\n",strerror(errno),errno);
        exit(0);
    }

    printf("==================== Waiting For Client's Request ====================\n");

    while(1){
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        //if((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) == -1){
        if((connfd = accept(listenfd, (struct sockaddr *)&client, &len)) == -1){
            printf("Accept socket error:%s(errorno:%d)\n",strerror(errno),errno);
            continue;
        }

        Client_TypeDef *ct = malloc(sizeof(Client_TypeDef));
        if(ct == NULL){
            printf("No space to create an client descriptor.\n");
        }
        //current_client = ct;

        //clear
        client_data_init(ct);
        //memset(ct, 0,sizeof(Client_TypeDef));

        //Create an new client
        add_new_client(ct);
        //Add client info.
        current_client->conn_fd = connfd;
        current_client->ip = client.sin_addr.s_addr;
        current_client->port = client.sin_port;
        current_client->num = client_num++;
/*
        //create a new thread to process
        char *from = inet_ntoa(client.sin_addr);
        printf("[%s:%d] Connected !\n", from, client.sin_port);
        n = recv(connfd, buff, VSIM_SOCKET_BUF_LEN, 0);
        buff[n] = '\0';

        printf("Recv msg from client[%s:%d]:%s\n", from, client.sin_port, buff);
        close(connfd);*/
    }

    close(listenfd);
}

/**
 * @brief main
 * for test VSIM managerment.
 * @param argc :none
 * @param argv :none
 * @return 0
 */
int main(int argc, char *argv[])
{

    VSIM_Managerment();

    return 0;
}

/*
pthread_t id[3];

typedef void (*VoidFuncVoid)(void);

VoidFuncVoid f_array[3];

void fun1(void)
{
    int i;
    for(i = 0;i < 3;i++){
        printf("I'm pthread [%s].\n",__func__);
    }
}

void fun2(void)
{
    int i;
    for(i = 0;i < 3;i++){
        printf("I'm pthread [%s].\n",__func__);
    }
}

void fun3(void)
{
    int i;
    for(i = 0;i < 3;i++){
        printf("I'm pthread [%s].\n",__func__);
    }
}

void create_thread(void)
{
    int i,ret;
    for(i = 0;i < 3;i++){
        ret = pthread_create(&id[i], NULL, (void *)f_array[i], NULL);
        if(ret != 0){
            printf("Create pthread error.\n");
            exit(1);
        }
    }
}

int main(int argc,char *argv[])
{
    int i,ret;


    f_array[0] = &fun1;
    f_array[1] = &fun2;
    f_array[2] = &fun3;

    create_thread();

    for(i = 0;i < 3;i++)printf("This is the main process.\n");
    for(i = 0;i < 3;i++)pthread_join(id[i],NULL);

    return 0;
}
*/

/*
int main(int argc, char *argv[])
{
    int listenfd, connfd;
    struct sockaddr_in servaddr;
    char buff[MAXLINE];
    int n;

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        printf("Create socket error:%s(errorno:%d)\n",strerror(errno),errno);
        exit(0);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    if(bind(listenfd, (struct sockaddr *)&servaddr,sizeof(servaddr)) == -1){
        printf("Bind socket error:%s(errorno:%d)\n",strerror(errno),errno);
        exit(0);
    }

    if(listen(listenfd, MAX_LINKS) == -1){
        printf("Listen socket error:%s(errorno:%d)\n",strerror(errno),errno);
        exit(0);
    }

    printf("==================== Waiting For Client's Request ====================\n");

    while(1){
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        //if((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) == -1){
        if((connfd = accept(listenfd, (struct sockaddr *)&client, &len)) == -1){
            printf("Accept socket error:%s(errorno:%d)\n",strerror(errno),errno);
            continue;
        }

        char *from = inet_ntoa(client.sin_addr);
        printf("[%s:%d] Connected !\n", from, client.sin_port);
        n = recv(connfd, buff, MAXLINE, 0);
        buff[n] = '\0';

        printf("Recv msg from client[%s:%d]:%s\n", from, client.sin_port, buff);
        close(connfd);
    }

    close(listenfd);

    return 0;
}
*/
