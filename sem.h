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
    void signal(){
        sem_post(&semImpl);
    }
    ~sem(){
        assert(sem_destroy(&semImpl) == 0);
    }
private:
    sem_t semImpl;
};