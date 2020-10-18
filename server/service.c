#include "work.h"
#include "tpool.h"

int main()
{
    printf("#####service\n");


    int port = 8000;\





    //创建线程池
    int ret = tpool_create(THREAD_NUM);
    if(ret != 0)
    {
        printf("create fail");
        exit(-1);
    }
    printf("---thread poll satrt---\n");

    //初始化service，监听请求
    int listenfd = Service_init(port);
    socklen_t sockaddr_len = sizeof(struct sockaddr);


    //epool 多路io转接
    //创建红黑树节点
    int epfd = epoll_create(EPOLL_SIZE); //生成用于处理accept的epoll专用的文件描述符
    struct epoll_event ev, events[EPOLL_SIZE];
    ev.data.fd = listenfd;

    //设置用于注册的读操作事件
     ev.events = EPOLLIN;

    //将listenfd添加到红黑树上
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev); /* 添加 */

    while(1)
    {
        int event_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);

        //接收连接，添加work到work-Queue
        for (int i = 0; i < event_count; ++i)
        {
            if (events[i].data.fd == listenfd)
            {
                int connfd;
                struct sockaddr_in clientaddr;

                while((connfd = accept(servSock, (sockaddr *)&clientaddr, (socklen_t *)&socklen)) > 0)
                {
                    printf("EPOLL: Received New"Connection);

                    struct args *p_args = (struct args*)malloc(sizeof(struct args));
                    //通信句柄
                    p_args->fd = connfd;
                    //回调函数
                    p_args->recv_finfo = recv_fileinfo;
                    p_args->recv_data = recv_filedata;


                     //添加work到work-Queue
                    tpool_add_work(work, (void*)p_args);
                }
            }
        }
    }
    return 0;
}