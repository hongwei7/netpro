#include<iostream>
#include<unistd.h>
#include<stdlib.h>
#include <iostream>
#include"server.h"

const int SERVPORT = 9999;

char serbuffer[BUFSIZ];
int writeSize = 0;

void get(char* buf, int size, tcpconn* wk) {
    std::cout << wk->getfd() << " is sending:" << std::endl;
    std::cout << "----output-----" << std::endl;
    std::cout << buf ;
    std::cout << "---------------" << std::endl;
	dbg("writing back:");
}

char* answer(int* size, tcpconn* wk){
	writeSize = read(STDIN_FILENO, serbuffer, sizeof(serbuffer));
    *size = writeSize;
    return serbuffer;
}

void httpRequest(char* buf, int size, tcpconn* wk){
    std::cout << "----request-----" << std::endl;
    std::cout << buf ;
    std::cout << "---------------" << std::endl;
    // dbg("REQUEST");
}

char httpres[] = "HTTP/1.1 200 OK\r\nDate: Sat, 31 Dec 2005 23:59:59 GMT\r\nContent-Type: text/html;charset=ISO-8859-1\r\n\r\n<html><head><title>TEST</title></head><body>HELLO</body></html>\n";
char* httpResponse(int* size, tcpconn* wk){
    // dbg("ANSWER");
    *size = strlen(httpres);
    return httpres;
}

int main() {
    server ser(SERVPORT, httpRequest, httpResponse);
    ser.mainloop();
}
