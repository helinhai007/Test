#include "work.h"

//gconn[]数组存放链接信息，带互斥锁
//闲置id
int freeid = 0;
//CONN_MAX,支持的最大连接数
struct conn gconn[CONN_MAX];
//初始化锁
pthread_mutex_t conn_lock =  PTHREAD_MUTEX_INITIALIZER; 

//结构体长度
//文件信息长度
int fileinfo_len = sizeof(struct fileinfo);
//分块头部信息长度
int head_len = sizeof(struct head);
//通信信息
int conn_len = sizeof(struct conn);

//初始化通信链接
int Service_init(int port)
{
    //获得socket通信句柄
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);   
    if(listen_fd == -1)
    {
        fprintf(stderr, "Create server socket failed.");
        exit(-1);
    }

    //设置非阻塞
    set_fd_noblock(listen_fd);

    //端口复用
    int opt = 1;
    int ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(ret == -1)
    {
        perror("setsockopt error");
        exit(-1);
    }

    struct sockaddr_in server_addr;
    socklen_t sockaddr_len = sizeof(struct sockaddr_in);
    bzero(&server_addr, sockaddr_len);
    server_addr.sin_family=AF_INET;   //ipv4协议族
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    server_addr.sin_port=htons(port);

    ret = bind(listen_fd,(struct sockaddr*)&server_addr,sockaddr_len);
    if(ret == -1)
    {
        fprintf(stderr, "server bind failed.");
        exit(-1);
    }

    ret = listen(listen_fd, LISTEN_QUEUE_LEN);
    if(ret == -1)
    {
        fprintf(stderr, "server bind failed.");
        exit(-1);
    }

    //返回通信句柄
    return listen_fd;
}


//设置套接字非阻塞
void set_fd_noblock(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0); 
     fcntl(fd, F_SETFL, flag | O_NONBLOCK); 

     return;
}




//创建文件
int createfile(char* filename, int size)
{
    //打开文件，设置权限
    int fd = open(filename, O_RDWR | O_CREAT);
    if(fd == -1)
    {
        perror("create file error");
        exit(-1);
    }

    //可以更改现有文件的访问权限
    int ret = fchmod(fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(ret == -1)
    {
        perror("fchmod error");
        exit(-1);
    }

    //移动文件指针到末尾-1的位置
    lseek(fd, size - 1, SEEK_SET);

    //在文件末尾写
    write(fd, "", 1);

    //关闭文件
    close(fd);

    //返回
    return 0;
}


//接收文件添加信息，添加链接到globalconn[]数组，创建填充文件，map到内存
void recv_fileinfo(int sockfd)
{
    //接收文件信息
    char fileinfo_buf[100] = {0};
    bzero(fileinfo_buf, fileinfo_len);


    //结束条件：刚好把整个结构体读满
    for(int n = 1; n < fileinfo_len; n++)
    {
        recv(sockfd, &fileinfo_buf[n], 1, 0);
    }
    
    struct fileinfo finfo;
    bzero(finfo, fileinfo_len);
    memcpy(&finfo, fileinfo_buf, fileinfo_len);


    //结构体内容打印检测
    printf("-------fileinfo------\n");
    printf("filename = %s\n filesize = %d\n count = %d\n bs = %d\n", finfo.filename, finfo.filesize,finfo.count,finfo.bs);

    printf("-------fileinfo------\n");

    //创建填充文件，mmap到虚拟共享内存
    char filename[100] = {0};
    //接受的文件名
    strcpy(filename, finfo.filename);
    //创建填充文件
    createfile(filename, finfo.filesize);

    //打开文件，maode:O_RDWR
    int fd = open(filename, O_RDWR);
    if(fd == -1)
    {
        printf("open file error");
        exit(-1);
    }

    //共享内存
    char *map = (char*)mmap(NULL, finfo.filesize, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    close(fd);

    //向gconn[]中添加链接
    pthread_mutex_lock(&conn_lock);
    printf("recv_fileinfo():Lock conn_lock,enter gconn[]\n");

    //查看使用标记，1、代表已经使用，0代表可用
    while(gconn[freeid].used)
    {
        freeid++;
        freeid = freeid% CONN_MAX;//类似循环队列处理，不处理freeid会大于最大值
    }

    //结构体赋值
    gconn[freeid].info_fd = sockfd;
    //清空filename[FILENAME_MAXLEN]数据，并赋值
    bzero(&gconn[freeid].filename, FILENAME_MAXLEN);
    //文件信息
    strcpy(gconn[freeid].filename, finfo.filename);
    gconn[freeid].filesize = finfo.filesize;
    gconn[freeid].count = finfo.count;
    gconn[freeid].bs = finfo.bs;
    //指针位置(共享内存的位置)
    gconn[freeid].mbegin = map;
    //已接受块的数量
     gconn[freeid].recvcount = 0;
    //标识符位置
     gconn[freeid].used = 1;

     //解锁
     pthread_mutex_ulock(&conn_lock);
     printf("recv_fileinfo();\n");

     //向client发送分配的freeid(gconn[]数组下标)，作为确认，每个块都将携带id
     char freeid_buf[INT_SIZE] = {0};
     memcpy(freeid_buf, &freeid, INT_SIZE);
     send(sockfd, freeid_buf, INT_SIZE, 0);
     printf("freeid=%d\n", *((int*)freeid_buf));

     return;
}


//接收文件快
void recv_filedata(int sockfd)
{


    //读取分块头部信息
    char head_buf[100] = {0};
    char *p = head_buf;

    //接受数据的大小
    int recv_size = 0;

    //读取分块文件头部信息
    while(1)
    {
        if(recv(sockfd, p, 1, 0) == 1)
        {
            ++recv_size;
            if(recv_size == head_len)
            {
                break;
            }
            ++p;
        }
    }

    //w文件头部结构体
    struct head fhead;
    //将读到的分块头部信息拷贝到fhead结构体中
    memcpy(&fhead, head_buf, head_len);

    //分块所属文件id
    int recv_id = fhead.id;

    //计算本快在map中起始地址fp
    int recv_offset = fhead.offset;
    char *fp = gconn[recv_id].mbegin + recv_offset;



    //检测
    printf("-----blcokhead------\n");

    printf("--------------------\n");



    //接收数据
    //接收数据，往map内存中写
    int remain_size = fhead.bs; //数据快中带接收数据的大小
    int size = 0;
    while(remain_size > 0)
    {
        if((size = recv(sockfd, fp, RECVBUF_SIZE, 0)) > 0)
        {
            fp +=size;
            remain_size -= size;
        }
    }
    printf("-------Recv a fileblock-------\n");


    //增加recv_count
    pthread_mutex_lock(&conn_lock);

    //将接收到的文件快加1
    gconn[recv_id].recvcount++;

    //判断是否是最后一个分块，如果是就同步map与文件，释放gconn
    if(gconn[recv_id].recvcount == gconn[recv_id].count)
    {
        //释放建立的映射区，类似malloc/free
        munmap((void*)gconn[recv_id].mbegin, gconn[recv_id].filesize);

        printf("-------Recv a File-----\n");

        int fd = gconn[recv_id].info_fd;

        //关闭
        close(fd);

        //j将gconn[].清空，尤其是used
        bzero(&gconn[recv_id], conn_len);
    }
    //解锁
    pthread_mutex_ulock(&conn_lock);

    //关闭句柄
    close(sockfd);


    return;
}

//工作线程分析，分析type，选择工种 
void *worker(void *arg)
{
    struct args *pw = (struct args*)arg;
    //通信句柄
    int connfd = pw->fd;

    //
    char typebuf[INT_SIZE] = {0};
    char *p = typebuf;

    //接收的字节大小
    int recv_size = 0;
    while(1)
    {
        if(recv(connfd, p, 1, 0) == 1)
        {
            ++recv_size;
            if(recv_size == INT_SIZE)
            {
                break;
            }
            ++p;
        }
    }

    //解除type的值
    int type = *((int *)typebuf);
    switch (type)
    {
        //接收文件信息
    case 0:
        {
            printf("##recv file_info: %d", type);
            pw->recv_info(connfd);
        }
        break;
        //接收文件快
    case 255:
        {
            printf("##recv file_info: %d", type);
            pw->recv_data(connfd);
        }
        break;
    
    default:
        printf("unknow type");
        return NULL;
    }
    return NULL;
}
