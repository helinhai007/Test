#include "tpool.h"
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>

static tpool_t *tpool = NULL;

//工作者线程函数，从任务链表中取出服务并执行
static void *thread_routine(void *arg)
{
    while(1)
    {
        pthread_mutex_lock(&tpool->queue_lock);
        //如果任务队列为空，且线程池关闭，线程阻塞等待任务
        while(!tpool->queue_head && !tpool->shutdown)
        {
             pthread_cond_wait(&tpool->queue_ready, &tpool->queue_lock);
        }

        //查看线程池开关，如果线程池关闭，线程退出
        if(tpool->shutdown)
        {
            pthread_mutex_unlock(&tpool->queue_lock);
            pthread_exit(NULL);
        }


        //从任务链表中取出任务，执行任务
        tpool_work_t *work = tpool->queue_head;
        tpool->queue_head = tpool->queue_head->next;

        //解锁
        pthread_mutex_unlock(&tpool->queue_lock);

        work->routine(work->arg);

        //线程完成任务后，释放任务
        free(work->arg);
        free(work);
    }
}

//创建线程池
int tpool_create(int max_thr_num)
{
    tpool = calloc(1, sizeof(tpool_t));
    if(!tpool)
    {
        printf("calloc tpool fail");
        exit(-1);
    }

    //初始化线程函数
    tpool->max_thr_num = max_thr_num;

    //初始化关闭信号
    tpool->shutdown = 0;

    //初始化任务链表
    tpool->queue_head = NULL;
    tpool->queue_tail = NULL;




    //初始化互斥锁
    if(pthread_mutex_init(&tpool->queue_lock, NULL) != 0)
    {
        printf("mutex_lock error\n");
        exit(-1);
    }
    //初始化条件变量
    if(pthread_cond_init(&tpool->queue_ready, NULL) != 0)
    {
        printf("conn_lock\n");
        exit(-1);
    }

    //创建work线程
    tpool->thr_id = calloc(max_thr_num, sizeof(pthread_t));
    if(!tpool->thr_id)
    {
        printf("calloc thr_id\n");
        exit(-1);
    }

    for(int i = 0; i < max_thr_num; i++)
    {
        int ret = pthread_create(&tpool->thr_id[i], NULL, thread_routine, NULL);
        if(ret != 0)
        {
            printf("pthread_create error\n");
            exit(-1);
        }
    }

    return 0;
}

//销毁线程池
void tpool_destory()
{
    if(tpool->shutdown)
    {
        return;
    }

    //关闭线程池开关
    tpool->shutdown = 1;

    //唤醒所有阻塞线程
    pthread_mutex_lock(&tpool->queue_lock);
    pthread_cond_broadcast(&tpool->queue_ready);
    pthread_mutex_unlock(&tpool->queue_lock);

    //释放thredId数组
    free(tpool->thr_id);

    //释放未完成的任务
    tpool_work_t *member = NULL;
    while(tpool->queue_head)
    {
        member = tpool->queue_head;
        tpool->queue_head = tpool->queue_head->next;
        free(member->arg);
        free(member);
    }

    //销毁互斥量，条件变量
    pthread_mutex_destroy(&tpool->queue_lock);
    pthread_cond_destroy(&tpool->queue_ready);

    //释放进程池结构体
    free(tpool);
}

//向线程池中添加任务
int tpool_add_work(void *(*routine)(void *), void *arg)
{
    //work指向等待加入任务链表的任务
    tpool_work_t *work = NULL;

    if(!routine)
    {
        printf("%s:Invalid argument\n", __FUNCTION__);
        return -1;
    }

    work = (tpool_work_t *)malloc(sizeof(tpool_work_t));
    if(!work)
    {
        printf("%s:malloc failed\n", __FUNCTION__);
        return -1;
    }

    //对任务队列的每一个任务进行初始化
    work->routine = routine;
    work->arg = arg;
    work->next = NULL;

    //将任务节点添加到任务链表
    pthread_mutex_lock(&tpool->queue_lock);
    //任务队列为空
    if(!tpool->queue_head)
    {
        tpool->queue_head = work;
        tpool->queue_tail = work;
    }
    //任务队列非空，查询任务队列末尾
    else
    {
        tpool->queue_tail->next = work;
        tpool->queue_tail = work;
    }

    //通知工作线程有新任务添加
    pthread_cond_signal(&tpool->queue_ready);
    //解锁
    pthread_mutex_unlock(&tpool->queue_lock);

    return 0;
}