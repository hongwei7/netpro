#ifndef __NETPRO__THREAD__H
#define __NETPRO__THREAD__H

#include <pthread.h>
#include <assert.h>
#include "noncopyable.h"

class thread: public noncopyable{
public:
    thread(void*(*routine)(void*), void* args){
        assert(pthread_create(&pid, nullptr, routine, args) == 0);
    }
    virtual ~thread(){
        pthread_exit(nullptr);
    }
private:
    pthread_t pid;
};

#endif