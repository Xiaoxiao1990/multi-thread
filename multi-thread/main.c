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
#include <sys/select.h>
#include <arpa/inet.h>          //ntoa()
#include <unistd.h>             //close()
#include <malloc.h>
#include <pthread.h>

#define VSIM_NAME_LEN           16
#define VSIM_SOCKET_BUF_LEN     1024
#define SERVER_PORT             6666
#define MAX_LINKS               720
#define CLIENT_TIME_OUT_VALUE   30          //seconds.
/**
 * @brief The Client_Info struct
 * A forward direction loop link node definition.
 */
typedef struct S_Client_Info{
    pthread_t tid;
    uint16_t num;
    //in_addr_t ip;
    struct in_addr ip;
    //struct in_addr sin_addr;
    in_port_t port;
    int conn_fd;
    uint8_t device_name[VSIM_NAME_LEN];
    uint16_t time_out;                      //unit second.
    struct S_Client_Info *prev,*next;
}Client_TypeDef;

Client_TypeDef *current_client;

/***
 * task managerment
 *
 * */
pthread_t pid_vsim_manager,pid_clients[MAX_LINKS];
pthread_mutex_t sw_client;
uint16_t Clients_CNT = 0;

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
    ct->ip.s_addr = 0;
    ct->port = 0;
    ct->time_out = 0;                           //clear
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
    printf("Add client[%d] to data link.\n",current_client->num);
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

    //backward one step in this case. avoid current_client pointer to NULL ,but the links have some more node.
    if(ct == current_client)current_client = current_client->prev;

    while(dct != ct){
        dct = dct->prev;
        if(dct == current_client){
            printf("Client [%d] not found !\n",ct->num);
            goto Node_Not_Found;
        }
    }
    //Delete this node dct.
    dct->prev->next = dct->next;
    dct->next->prev = dct->prev;

//    if((dct == dct->next) && (dct == dct->prev));//Needn't ?
//   else{
        free(dct);
        dct = NULL;
//    }

Node_Not_Found:;
}

/**
 * @brief delete_client_by_num
 * delet client by client number
 * @param num
 * client number
 */
void delete_client_by_num(uint16_t num)
{
    Client_TypeDef *dct = current_client; //backward search from current_client

    //find out the node first
    while(dct->num != num){
        dct = dct->prev;
        if(dct == current_client){
            printf("Client [%d] not found !\n",num);
            goto Node_Not_Found;
        }
    }
    //ct is the destination node delete it
    delete_client_by_pos(dct);

Node_Not_Found:;
}

/**
 * @brief client_stuff
 * @param arg
 * @return
 */
void * client_stuff(void *arg)
{
    Client_TypeDef *ct = (Client_TypeDef *)arg;
    fd_set readfds;
    struct timeval time_out = {0,0};
    char buff[VSIM_SOCKET_BUF_LEN];
    int n;

    char *from = inet_ntoa(ct->ip);
    printf("[%s:%d] Connected !\n", from, ct->port);

//    FD_ZERO(&readfds);
//    FD_SET(ct->conn_fd,&readfds);

    do{
        FD_ZERO(&readfds);
        FD_SET(ct->conn_fd,&readfds);

        n = select(ct->conn_fd + 1, &readfds, NULL, NULL, &time_out);//no block

        if(n == 0){
            //time out
            if(ct->time_out == 0)break;
        }
        else if(n < 0){
            //error
            break;
        }
        else{
            if(FD_ISSET(ct->conn_fd ,&readfds)){
                n = recv(ct->conn_fd, buff, VSIM_SOCKET_BUF_LEN, 0);
                if(n > 0){
                    buff[n] = '\0';
                    printf("[%s:%d]Client[%d] says:%s\n", from, ct->port, ct->num, buff);
                    ct->time_out = CLIENT_TIME_OUT_VALUE;
                }
                else{
                    printf("Client[%d] socket closed.\n", ct->num);
                    ct->time_out = 0;
                    break;         //socket closed.
                }
            }
        }
        //printf("Client[%d]:time out after %2d second(s).\n", ct->num, ct->time_out);
    }while(ct->time_out > 0);

    char *msg = "You're out!";

    send(ct->conn_fd, msg, strlen(msg), 0);
    //Client link time out.
    sleep(2);//wait for mssage send out!
    close(ct->conn_fd);

    //Client OFF line
    printf("[%s:%d]Client[%d] off line.\n", from, ct->port, ct->num);

    //delete the client data node
    pthread_mutex_lock(&sw_client);

    delete_client_by_pos(ct);
    if(Clients_CNT > 0)Clients_CNT--;

    pthread_mutex_unlock(&sw_client);
    //exit thread
    pthread_exit(NULL);
}

/**
 * @brief VSIM_Managerment
 * manager the clients
 */
void VSIM_Managerment(void)
{
    int listenfd, connfd;
    struct sockaddr_in servaddr;

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

    pthread_mutex_init(&sw_client, NULL);

    while(1){
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        //if((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) == -1){
        if((connfd = accept(listenfd, (struct sockaddr *)&client, &len)) == -1){
            printf("Accept socket error:%s(errorno:%d)\n",strerror(errno),errno);
            continue;
        }

        Client_TypeDef *ct = (Client_TypeDef *)malloc(sizeof(struct S_Client_Info));
        if(ct == NULL){
            printf("No space to create an client descriptor.\n");
            //ERROR
            continue;
        }
        //current_client = ct;

        //clear
        client_data_init(ct);
        //memset(ct, 0,sizeof(Client_TypeDef));

        //Create an new client
        //Add client info.
        ct->conn_fd = connfd;
        ct->ip.s_addr = client.sin_addr.s_addr;
        ct->port = client.sin_port;
        ct->time_out = CLIENT_TIME_OUT_VALUE;

        if(Clients_CNT > 0){
            uint16_t flag;
            uint16_t c_num;
            Client_TypeDef *dct;
            for(c_num = 1; c_num != 0; c_num++)
            {//find a number not been used.
                flag = 1;
                dct = current_client;
                do{
                    if(c_num == dct->num)
                    {
                        flag = 0;
                        break;
                    }else dct = dct->next;
                }while(current_client != dct);

                if(flag)break;
            }
            ct->num = c_num;
            add_new_client(ct);//current_client point to ct
        }
        else{//header
            ct->num = Clients_CNT;
            ct->next = ct;
            ct->prev = ct;
            current_client = ct;
        }
        //clients counter
        Clients_CNT++;
        int ret = 0;

        ret = pthread_create(&current_client->tid, NULL, &client_stuff, current_client);
        if(ret != 0){
            printf("Create pthread error.\n");
            exit(1);
        }
        pthread_detach(current_client->tid);
//       if((pthread_create(&pid_clients[client_num-1], (void *)NULL, (void *)client_stuff, NULL)) < 0){
 //          printf("Create pthread error.\n");
  //         exit(1);
  //     }
        /*
        char buff[VSIM_SOCKET_BUF_LEN];
        int n;
        //create a new thread to process
        char *from = inet_ntoa(client.sin_addr);
        printf("[%s:%d] Connected !\n", from, client.sin_port);
        n = recv(connfd, buff, VSIM_SOCKET_BUF_LEN, 0);
        buff[n] = '\0';

        printf("Recv msg from client[%s:%d]:%s\n", from, client.sin_port, buff);
        close(connfd);
        */
    }

    close(listenfd);
}

/**
 * @brief SYS_Time_Wheel
 * Process all the timer.As client on line timer.
 * Timer:1s
 */
void * SYS_Time_Wheel(void *arg)
{
    arg = arg;

    while(1)
    {
        //Client on line Timer
        //pthread_mutex_lock(&sw_client);

        Client_TypeDef *ct = current_client;

        while(ct != NULL){
            if(ct->time_out > 0)
            {
                ct->time_out--;
                printf("[%s:%d]Client[%d]:Time out after (%d) second(s).\n", inet_ntoa(ct->ip), ct->port, ct->num, ct->time_out);
            }
            ct = ct->prev;
            if(ct == current_client)break;
        }
        //pthread_mutex_unlock(&sw_client);
        //every seconds
        sleep(1);
    }
    printf("Error:%s\n",__func__);
    pthread_exit(NULL);;
}
/*
void Client_Init(void)
{
    //Initalize head of client data
    current_client = (Client_TypeDef *)malloc(sizeof(struct S_Client_Info));
    client_data_init(current_client);
    current_client->next = current_client;
    current_client->prev = current_client;
}
*/
/**
 * @brief main
 * for test VSIM managerment.
 * @param argc :none
 * @param argv :none
 * @return 0
 */
int main(int argc, char *argv[])
{
    pthread_t time_wheel;
    int ret;

    //Client_Init();

    ret = pthread_create(&time_wheel, NULL, &SYS_Time_Wheel, NULL);
    if(ret != 0){
        printf("Create pthread error.\n");
        exit(1);
    }
    pthread_detach(time_wheel);

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
