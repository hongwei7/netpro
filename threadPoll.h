#ifndef __NETPRO_THREADPOLL_H
#define __NETPRO__THREADPOLL__H

#include <vector>
#include <pthread.h>
#include "mutex.h"
#include "cond.h"

class threadPollTask{
public:
    void*(*task)(void*);
    void* args;
};

class threadPoll: noncopyable{
public:
    threadPoll(int initNum)
    : locker(), threadCount(0), queueNotFull(&locker), queueNotEmpty(&locker), 
    mainThread(pthread_self()), threadsList(initNum, 0), taskQueue(initNum, nullptr){
    }
private:
    mutex locker;
    int threadCount;                        //当前忙的线程个数
    cond queueNotFull;                      //当任务队列满时，添加任务的线程阻塞，等待此条件变量
    cond queueNotEmpty;                     //任务队列不空，通知工作线程处理工作

    std::vector<pthread_t> threadsList;     //线程号数组
    pthread_t mainThread;                   //线程池控制线程
    std::vector<threadPollTask*> taskQueue; //任务队列(环形)

    int minThreadNum;                       //最小线程数
    int maxThreadNum;                       //最大线程数
    int liveThreadNum;                      //存活线程个数
    int busyThreadNum;                      //忙状态线程个数
    int waitExitNum;                        //等待销毁的线程个数

    int queueFront;                         //taskQueue的头部
    int queueBack;                          //taskQueue的尾部
    int queueSize;                          //环形队列的实际任务数
    int queueMaxSize;                       //环形队列的实际大小

    bool shutdown;                          //线程池是否已经关闭
};

#endif