#include<iostream>
#include<unistd.h>
#include<stdlib.h>
#include <iostream>
#include"server.h"
#include "threadPool.h"

const int SERVPORT = 9999;
void echo(char* buf, int size, tcpconn* wk) {
    std::cout << wk->getfd() << " is sending:" << std::endl;
    std::cout << "----output-----" << std::endl;
    std::cout << buf << std::endl;
    std::cout << "---------------" << std::endl;
}

int iii = 0;
void* test(void* arg){
    if(iii++ % 100 == 0) dbg(iii);
    sleep(1);
}

int main() {
    threadPool pool(2, 1000, 20);
    for(int i = 0; i < 10000; ++i){
        pool.threadPoolAdd(test, nullptr);
    }
    sleep(20);
    // dbg(iii);
    exit(0);
    // server ser(SERVPORT, echo);
    // ser.mainloop();
}
