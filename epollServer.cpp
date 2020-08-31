#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdarg.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <sys/epoll.h>
#include <fcntl.h>  

#define BUF_SIZE 1024

using namespace std;

void setnonblocking(int sock)  
{  
    int opts;  
     opts=fcntl(sock,F_GETFL);  
    if(opts<0)  
    {  
        perror("fcntl(sock,GETFL)");  
        //exit(1);  
    }  
     opts = opts|O_NONBLOCK;  
    if(fcntl(sock,F_SETFL,opts)<0)  
    {  
        perror("fcntl(sock,SETFL,opts)");  
        //exit(1);  
    }  
}  

int main()
{

    int epfd, nfds;
    //声明epoll_event结构体的变量,ev用于注册事件,数组用于回传要处理的事件
    struct epoll_event ev, events[20];

    //创建一个epoll的句柄，size用来告诉内核这个监听的数目一共有多大
    epfd = epoll_create(256); //生成用于处理accept的epoll专用的文件描述符

    int servSock = socket(AF_INET, SOCK_STREAM, 0);


    //把socket设置为非阻塞方式  
    //setnonblocking(listenfd);  

    //设置与要处理的事件相关的文件描述符
    ev.data.fd = servSock;

    //设置要处理的事件类型
    ev.events = EPOLLIN | EPOLLET;

    //注册epoll事件
    epoll_ctl(epfd, EPOLL_CTL_ADD, servSock, &ev);

    sockaddr_in sockAddr;

    memset(&sockAddr, 0, sizeof(sockAddr));

    sockAddr.sin_family = AF_INET;

    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    sockAddr.sin_port = htons(1234);

    if (bind(servSock, (const struct sockaddr *)&sockAddr, sizeof(sockAddr)) != 0)
    {
        cout << "bind failed" << endl;
        return -1;
    }
    cout << "bind sucess" << endl;

    if (listen(servSock, 20) != 0)
    {
        cout << "listen failed" << endl;
        return -1;
    }
    cout << "listen sucess" << endl;

    int socklen;
    struct sockaddr_in clientaddr; //客户端的地址信息

    int clntSock = accept(servSock, (struct sockaddr *)&clientaddr, (socklen_t *)&socklen);
    if (clntSock == -1)
    {
        cout << "accept failed" << endl;
        return 0;
    }

    string str;
    str = "HTTP/1.1 200 OK\r\n";
    str = str + "Access-Control-Allow-Origin: null\r\n";
    str = str + "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n";
    str = str + "Access-Control-Allow-Headers: X-PINGOTHER, Content-Type\r\n";
    str = str + "Content-Encoding: gzip, deflate\r\n";
    str = str + "Connection: Keep-Alive\r\n";
    cout << str << endl;

    int nCount1 = send(clntSock, str.c_str(), str.size(), 0);
    cout << "accept sucess" << endl;

    if (nCount1 <= 0)
    {
        cout << "send: failed" << endl;
        return 0;
    }
    cout << "send: sucess" << endl;

    close(clntSock);

    int connfd;
    int sockfd;
    int n;
    char line[1024];
    FILE *fp;

    while (1)
    {
        //等待epoll事件的发生
        //返回需要处理的事件数目nfds，如返回0表示已超时。
        nfds = epoll_wait(epfd, events, 20, 500);
        cout << "epoll_wait" << endl;

        //处理所发生的所有事件
        for (int i = 0; i < nfds; ++i)
        {
            //如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。
            if (events[i].data.fd == servSock)
            {
                cout << "connect begin" << endl;
                connfd = accept(servSock, (sockaddr *)&clientaddr, (socklen_t *)&socklen);
                if (connfd < 0)
                {
                    perror("connfd < 0");
                    exit(1);
                }
                //setnonblocking(connfd);
                char *str = inet_ntoa(clientaddr.sin_addr);
                cout << "accapt a connection from " << str << endl;

                //设置用于读操作的文件描述符
                ev.data.fd = connfd;

                //设置用于注册的读操作事件
                ev.events = EPOLLIN | EPOLLET;

                //注册ev
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev); /* 添加 */
                cout << "connect end" << endl;
            }

            //如果是已经连接的用户，并且收到数据，那么进行读入。
            else if (events[i].events & EPOLLIN)
            {
                cout << "receive begin" << endl;
                cout << "EPOLLIN" << endl;
                if ((sockfd = events[i].data.fd) < 0)
                    continue;
                //循环接收数据，直到文件传输完毕
                char buffer[BUF_SIZE] = {0}; //文件缓冲区
                int nCount;
                if ((nCount = recv(sockfd, buffer, sizeof(buffer), 0)) < 0)
                {
                    // Connection Reset:你连接的那一端已经断开了，而你却还试着在对方已断开的socketfd上读写数据！
                    if (errno == ECONNRESET)
                    {
                        close(sockfd);
                        events[i].data.fd = -1;
                    }
                    else
                        std::cout << "readline error" << std::endl;
                }
                else if (n == 0) //读入的数据为空
                {
                    close(sockfd);
                    events[i].data.fd = -1;
                }

                cout << nCount << endl;
                string szTemp("");
                szTemp = "";
                szTemp += buffer;
                cout << szTemp;

                FILE *fp = fopen("123.txt", "wb"); //以二进制方式打开文件

                if (fp == NULL)
                {
                    printf("Cannot open file, press any key to exit!\n");
                    exit(0);
                }
                fwrite(buffer, nCount, 1, fp);
                cout << "fwrite: sucess" << endl;
                fclose(fp);

                close(connfd);

                //设置用于写操作的文件描述符
                ev.data.fd = sockfd;

                //设置用于注册的写操作事件
                ev.events = EPOLLOUT | EPOLLET;

                //修改sockfd上要处理的事件为EPOLLOUT
                epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev); /* 修改 */
                cout << "receive end" << endl;
            }
            else if (events[i].events & EPOLLOUT) // 如果有数据发送

            {
                cout << "send begin" << endl;
                sockfd = events[i].data.fd;
                string szTemp("");
                szTemp = "Server:" + szTemp + "\n";
                send(sockfd, szTemp.c_str(), szTemp.size(), 0);

                //设置用于读操作的文件描述符
                ev.data.fd = sockfd;

                //设置用于注册的读操作事件
                ev.events = EPOLLIN | EPOLLET;

                //修改sockfd上要处理的事件为EPOLIN
                epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev); /* 修改 */
                cout << "send end" << endl;
            }
        }
    }

    cout << "recv sucess" << endl;

    close(servSock);

    close(epfd);
    cout << "send end" << endl;

    return 0;
}
