#ifndef __NETPRO_THREADPOoL_H
#define __NETPRO__THREADPOoL__H

#include <vector>
#include <pthread.h>
#include "dbg.h"
#include "mutex.h"
#include "cond.h"

const unsigned int SLEEP_AND_MANAGE_SECS = 1;    //每隔多少秒管理一次线程池
int MIN_WAIT_THREAD_NUM = 5;                    //当等待的任务数大于该阈值，增加线程
int DEFAULT_THREAD_VARY = 10;                    //每次改变的线程数

class threadPoolTask{
public:
    threadPoolTask(void*(*fun)(void*), void*arg)
    :task(fun), args(arg){}
    void*(*task)(void*);
    void* args;
    void run(){
        task(args);
    }
};

void* threadPoolThread(void* args);
void* threadPoolManage(void* args);

class threadPool: noncopyable{
public:
    threadPool(int minThread,  int maxThread, int queueMax)
    : locker(), threadlocker(), queueNotFull(&locker), queueNotEmpty(&locker), 
    mainThread(pthread_self()), threadsList(maxThread, 0), taskQueue(queueMax, nullptr),
    queueFront(0), queueBack(0), queueSize(0), 
    minThreadNum(minThread), maxThreadNum(maxThread), liveThreadNum(minThread), busyThreadNum(0), waitExitNum(0), shutdown(false){
        for(int i = 0; i < minThreadNum; ++i){
            pthread_t &tid = threadsList[i];
            int ret = pthread_create(&tid, nullptr, threadPoolThread, (void*)this);
            assert(ret == 0);
        }
        assert(pthread_create(&mainThread, nullptr, threadPoolManage, (void*)this) == 0);
    }
    friend void* threadPoolThread(void* args){
        threadPool* pool = static_cast<threadPool*>(args);
        while(true){
            pool->locker.lock();
            while(pool->queueSize == 0 && !pool->shutdown){        //任务队列无任务
                pool->queueNotEmpty.wait();
                if(pool->waitExitNum > 0){                         //清理空闲线程
                    pool->waitExitNum--;
                    if(pool->liveThreadNum > pool->minThreadNum){  //可清理当前进程
                        pool->liveThreadNum --;
                        pool->locker.unlock();
                        pthread_exit(NULL);
                    }
                }
            }            
            if(pool->shutdown){
                pool->locker.unlock();
                pthread_exit(NULL);
            }

            //取出任务
            assert((pool->queueBack + pool->queueSize) % pool->taskQueue.size() == pool->queueFront);
            assert(pool->taskQueue[pool->queueBack] != nullptr);
            threadPoolTask task = *(pool->taskQueue[pool->queueBack]);
            delete pool->taskQueue[pool->queueBack];
            pool->taskQueue[pool->queueBack] = nullptr;
            pool->queueBack = (pool->queueBack + 1) % (pool->taskQueue.size());
            pool->queueSize--;
            pool->queueNotFull.boardcast();
            pool->locker.unlock();

            //执行任务
            pool->threadlocker.lock();
            pool->busyThreadNum++;
            pool->threadlocker.unlock();
            // dbg("task running...");
            task.run();

            //任务处理结束
            // dbg("task finished");
            pool->threadlocker.lock();
            pool->busyThreadNum--;
            // dbg(pool->queueFront, pool->queueBack, pool->queueSize);
            pool->threadlocker.unlock();
        }
        pthread_exit(NULL);
    }
    friend void* threadPoolManage(void* args){
        threadPool* pool = static_cast<threadPool*>(args);
        assert(pthread_self() == pool->mainThread);
        while(!pool->shutdown){
            sleep(SLEEP_AND_MANAGE_SECS);
            
            pool->locker.lock();
            int queueSize = pool->queueSize;
            int liveThreadNum = pool->liveThreadNum;
            pool->locker.unlock();

            pool->threadlocker.lock();
            int busyThreadNum = pool->busyThreadNum;
            pool->threadlocker.unlock();
            
            if(queueSize >= MIN_WAIT_THREAD_NUM && liveThreadNum < pool->maxThreadNum){   //创建新线程
                pool->locker.lock();
                int i;
                for(i = 0; i < DEFAULT_THREAD_VARY && liveThreadNum + i < pool->maxThreadNum; ++i){
                    int ret = pthread_create(&(pool->threadsList[i + liveThreadNum]), nullptr, threadPoolThread, (void*)pool);
                     assert(ret == 0);
                }
                pool->liveThreadNum += i;
                // dbg("线程增加");
                // dbg(pool->liveThreadNum);
                pool->locker.unlock();
            }

            if(busyThreadNum * 2 < liveThreadNum && liveThreadNum > pool->minThreadNum){   //销毁空闲进程
                pool->locker.lock();
                pool->waitExitNum += DEFAULT_THREAD_VARY;
                pool->locker.unlock();

                for(int i = 0; i < DEFAULT_THREAD_VARY; ++i){
                    pool->queueNotEmpty.signal();
                }
                // dbg("线程减少");
                // dbg(pool->liveThreadNum);
            }
        }
        return NULL;
    }

    int threadPoolAdd(void*(*function)(void*), void*arg){
        locker.lock();
        // dbg(queueFront, queueBack, queueSize);
        while(taskQueue.size() == queueSize && !shutdown){
            queueNotFull.wait();    //等待队列有位置
        }
        if(shutdown)locker.unlock();
        threadPoolTask* newTask = new threadPoolTask(function, arg);
        assert(taskQueue[queueFront] == nullptr);
        taskQueue[queueFront] = newTask;
        queueFront = (queueFront + 1) % taskQueue.size();
        queueSize++;
        queueNotEmpty.signal();
        locker.unlock();
        return 0;
    }
    int threadPoolDestroy(){
        if(shutdown == true)return -1;
        shutdown = true;
        pthread_join(mainThread, nullptr);
        for (int i = 0; i < liveThreadNum; ++i) { //唤醒所有的线程
            queueNotEmpty.boardcast();
        }
        for(auto &tid: threadsList){
            pthread_join(tid, nullptr);
        }
        return 0;
    }
    virtual ~threadPool(){
        if(!shutdown)threadPoolDestroy();
    }

private:
    mutex locker;                           //任务队列锁        
    mutex threadlocker;                     //线程池锁
    cond queueNotFull;                      //当任务队列满时，添加任务的线程阻塞，等待此条件变量
    cond queueNotEmpty;                     //任务队列不空，通知工作线程处理工作

    std::vector<pthread_t> threadsList;     //线程号数组
    pthread_t mainThread;                   //线程池控制线程
    std::vector<threadPoolTask*> taskQueue; //任务队列(环形)

    int minThreadNum;                       //最小线程数
    int maxThreadNum;                       //最大线程数
    int liveThreadNum;                      //存活线程个数
    int busyThreadNum;                      //忙状态线程个数
    int waitExitNum;                        //等待销毁的线程个数

    int queueFront;                         //taskQueue的头部
    int queueBack;                          //taskQueue的尾部
    int queueSize;                          //任务数量

    bool shutdown;                          //线程池是否已经关闭
};

#endif