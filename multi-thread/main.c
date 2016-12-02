/**************
 *
 * Server
 *
 * **********/
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

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
