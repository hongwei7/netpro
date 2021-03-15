#include<iostream>
#include<unistd.h>
#include<stdlib.h>
#include <iostream>
#include"server.h"

const int SERVPORT = 9999;

void httpRequest(char* buf, int size, tcpconn* wk){
    // std::cout << "----request-----" << std::endl;
    // std::cout << buf ;
    // std::cout << "---------------" << std::endl;
    // dbg("REQUEST");
}

int httpResponse(char* buf, int size, tcpconn* wk){
    char httpres[] = "HTTP/1.1 200 OK\r\nDate: Sat, 31 Dec 2005 23:59:59 GMT\r\nContent-Type: text/html;charset=ISO-8859-1\r\n\r\n<html><head><title>TEST</title></head><body>HELLO</body></html>\n";
    dbg("ANSWER");
    strcpy(buf, httpres);
    dbg("HTTPRES COPY");
    return sizeof(httpres);
}

int main() {
    sigset_t signal_mask;
    sigemptyset (&signal_mask);  
    sigaddset (&signal_mask, SIGPIPE);
    int rc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
    if (rc != 0)
    {
        printf("block sigpipe error\n");

    } 


    server ser(SERVPORT, httpRequest, httpResponse);
    ser.mainloop();
}
