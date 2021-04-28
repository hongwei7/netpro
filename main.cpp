#include<iostream>
#include<unistd.h>
#include<stdlib.h>
#include <iostream>
#include"server.h"

const int SERVPORT = 9999;

int main() {
    server<tcpconn> ser(SERVPORT);
    ser.mainloop();
}
