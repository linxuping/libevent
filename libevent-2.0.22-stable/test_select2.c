#include<netinet/in.h>
#include<sys/socket.h>
#include<stdio.h>
#include<strings.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<sys/select.h>
#include<sys/time.h>
#include<unistd.h>

#define BACKLOG 4
#define INET_ADDRSTRLEN 16

struct args{
    int arg1;
    int arg2;
};

struct result{
    int sum;
};

#define ARG struct args
#define RESULT struct result

int sum(ARG* args){
    return args->arg1 + args->arg2;
}

int main(int argc, char** argv)
{
    int  listenfd, connfd, maxi, maxfd, sockfd, i;
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr;
    char  buf[INET_ADDRSTRLEN];
    fd_set  rset, allset;
    int  nready, client[FD_SETSIZE];
    ARG args;
    RESULT result;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(8800);

    bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    listen(listenfd, BACKLOG); //un-block

    maxfd = listenfd; /*initialize*/
    maxi = -1;  /*index into client[] array*/
    for(i = 0; i < FD_SETSIZE; i++)
        client[i] = -1;  /*-1 indicates available entry*/
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    for( ; ; )
    {
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

        if(FD_ISSET(listenfd, &rset))  /*new client connection*/
        {
            len = sizeof(cliaddr);
            connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
            printf("accept fd \n");

            for(i = 0; i < FD_SETSIZE; i++)
                if(client[i] < 0)
                {
                    client[i] = connfd; /*save descripter*/
                    break;
                }
            if(i == FD_SETSIZE)
            {
                printf("too many clients");
                //exit(-1);
                return -1;
            }

            FD_SET(connfd, &allset);  /*add new descripter to set*/
            if(connfd > maxfd)
                maxfd = connfd;   /*for select*/
            if(i > maxi)
                maxi = i;   /*max index in client[] array*/
            if(--nready <= 0)
                continue;   /*no more readable descripters*/
        }

        for(i = 0; i <= maxi; i++)
        {
            if((sockfd = client[i]) < 0)
                continue;
            if(FD_ISSET(sockfd, &rset))
            {
                printf("connection from %s, port %d /n", inet_ntop(AF_INET, &cliaddr.sin_addr, buf, sizeof(buf)), htons(cliaddr.sin_port));
                recv(sockfd, (void *)&args, sizeof(ARG), 0);
                result.sum = sum((ARG*)&args);
                send(connfd, &result, sizeof(RESULT), 0);

                close(sockfd);
                FD_CLR(sockfd, &allset);
                client[i] = -1;

                if(--nready <= 0)
                    break;
            }
        }
    }
    close(listenfd);  
}
