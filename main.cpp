#include<iostream>
#include<unistd.h>
#include<stdlib.h>
#include <iostream>
#include"server.h"

const int SERVPORT = 9999;

int main() {
    server ser(SERVPORT);
    ser.mainloop();
}
