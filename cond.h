#ifndef __NETPRO__COND__H
#define __NETPRO__COND__H

#include <pthread.h>
#include "mutex.h"
#include "noncopyable.h"

class cond: noncopyable{
public:
    cond(mutex* mutexlock){condImpl = PTHREAD_COND_INITIALIZER;}
    void signal(){
        int ret = pthread_cond_signal(&condImpl);
        assert(ret == 0);
    }
    void wait(){
        int ret = pthread_cond_wait(&condImpl, m->getlock());
        assert(ret == 0);
    }
    ~cond(){
        int ret = pthread_cond_destroy(&condImpl);
        assert(ret == 0);
    }
private:
    pthread_cond_t condImpl;
    mutex* m;
};

#endif