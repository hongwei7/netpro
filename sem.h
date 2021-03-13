#include <semaphore.h>

class sem{
public:
    sem(){
        assert(sem_init(&semImpl, 0, 0) == 0);
    }
    sem(int t){
        assert(sem_init(&semImpl, 0, t) == 0);
    }
    void wait(){
        sem_wait(&semImpl);
    }
    int tryWait(){
        int ret = sem_trywait(&semImpl);
        if(ret == -1 && errno == EAGAIN)return 1; //block
        else if(ret == 0)return 0;                //unlock
        else perror("tryWait");
        assert(ret == 0);
        return -1;
    }
    int timeWait(time_t seconds, long nsec){
        // struct timespec {
        //        time_t tv_sec;      /* Seconds */
        //        long   tv_nsec;     /* Nanoseconds [0 .. 999999999] */
        //    };
        struct timespec spec = {seconds, nsec};
        int ret = sem_timedwait(&semImpl, &spec);
        if(ret == 0)return 0;                         //success
        else if(ret == -1 && errno == ETIMEDOUT)      //timeout
            return 1;
        else perror("timeWait");
        assert(ret == 0);
        return -1;
    }
    void signal(){
        sem_post(&semImpl);
    }
    ~sem(){
        assert(sem_destroy(&semImpl) == 0);
    }
private:
    sem_t semImpl;
};