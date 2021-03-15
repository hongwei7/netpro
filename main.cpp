#include<iostream>
#include<unistd.h>
#include<stdlib.h>
#include <iostream>
#include"server.h"

const int SERVPORT = 9999;

int main() {
    sigset_t signal_mask;
    sigemptyset (&signal_mask);  
    sigaddset (&signal_mask, SIGPIPE);
    int rc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
    if (rc != 0) printf("block sigpipe error\n");


    server ser(SERVPORT);
    ser.mainloop();
}
