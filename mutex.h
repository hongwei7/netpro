#ifndef __NETPRO__MUTEX__H
#define __NETPRO__MUTEX__H

#include<pthread.h>
#include<assert.h>
#include<errno.h>
#include "noncopyable.h"

class mutex: noncopyable{
public:
    mutex(){
        pthread_mutex_init(&mutex_, NULL);
    }
    int lock(){
        return pthread_mutex_lock(&mutex_);
    }
    int trylock(){
        int ret = pthread_mutex_trylock(&mutex_);
        if( ret == 0)return 0;               //success lock
        else if(ret == EBUSY) return 1;      //未可用
        else if(ret == EINVAL) return -1;    //失效
        else { 
            dbg(ret);
            abort();
        }
    }
    int unlock(){
        return pthread_mutex_unlock(&mutex_);
    }
    pthread_mutex_t* getlock(){return &mutex_;}
    virtual ~mutex(){
        pthread_mutex_destroy(&mutex_);
    }

private:
    pthread_mutex_t mutex_;
};

#endif