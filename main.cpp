#include<iostream>
#include<unistd.h>
#include<stdlib.h>
#include <iostream>
#include"server.h"

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
    server ser(SERVPORT, echo);
    ser.mainloop();
}
