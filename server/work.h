#ifndef _WORK_H__
#define _WORK_H__



#include <unistd.h>
#include <sys/types.h>
//#include <string.h>
//#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/stat.h> 
#include <fcntl.h>   
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h> 
#include <sys/epoll.h>
#include <sys/mman.h>

#define PORT 10000 //监听端口
#define THREAD_NUM 8;  //线程池数量
#define LISTEN_QUEUE_LEN 100 //listen队列长度
#define EPOOL_SIZE 50 //epoll最大监听fd数量
#define INT_SIZE 4 //int 长度
#define FILENAME_MAXLEN 30 //文件名最大长度
#define CONN_MAX 10 //支持最大连接数，一个链接包含多个socket链接(多线程)

//一次接收数据大小
#define RECVBUF_SIZE 65535

//tpool_add_work参数
struct args
{
    int fd;
    void (*recv_info)(int fd);
    void (*recv_data)(int fd);
};

//文件信息
struct fileinfo
{
    char filename[FILENAME_MAXLEN]; //文件名
    int  filesize;   //文件大小
    int  count;     //分块数量
    int bs;         //标准分块大小
};

//与客户端关联的链接，每次传输建立一个，在多线程之间共享
struct conn
{
    int info_fd;        //信息通信句柄socketfd;接收文件信息，文件传送通知client
    char filename[FILENAME_MAXLEN];
    int  filesize;   //文件大小
    int  count;     //分块数量
    int bs;         //标准分块大小

    int recvcount; //已接受快的数量，recv_count = count表示传输完毕
    char *mbegin;   //mmap起始地址

    int used;       //使用标记1表示已用，0表示可用
};


//分块头部
struct head
{
    char filaname[FILENAME_MAXLEN];
    int  id;   //文件所属的文件id，gconn[CONN_MAX]数组的下标，g(global);全局
    int  offset;     //分块在原文件的偏移
    int bs;         //标准分块大小
};

//初始化service,监听请求，返回listenfd
int Service_init(int port);

//设置非阻塞
void set_fd_noblock(int fd);

//读取文件信息
void recv_fileinfo(int sockfd);

//读取文件数据
void recv_filedata(int sockfd);

//tpool add work参数
void *worker(void *arg);

#endif
