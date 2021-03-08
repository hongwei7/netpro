#include<iostream>
#include<unistd.h>
#include<stdlib.h>
#include"server.h"

const int SERVPORT = 9999;

void echo(char* buf, int size){
    dbg(buf);
}

int main(){
    server ser(SERVPORT, nullptr);
    ser.mainloop();
}
