#ifndef __NETPRO__SOCKET__H
#define __NETPRE__SOCKET__H

#include<sys/socket.h>
#include "epoll.h"
#include<assert.h>
#include<sys/types.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include <stdlib.h>
#include <unistd.h>
#include "dbg.h"
void* worker(void* args) {
    // int clifd;
    // assert(clifd = accept(sockfd, (sockaddr*)&cliAddr, sizeof(cliAddr)));
    // dbg( ntohs(cliAddr.sin_port) );
    // event client(&epolltree, clifd, worker);
    // client.hangRD();
}

class server {
public:
    server(int port) : sockfd(socket(AF_INET, SOCK_STREAM, 0)) {
        assert(sockfd != 0);
        memset(&servAddr, 0, sizeof(servAddr));
        memset(&cliAddr, 0, sizeof(cliAddr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(port);
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        assert(bind(sockfd, (sockaddr*)&servAddr, sizeof(servAddr)) == 0);
        event host(&epolltree, sockfd, worker);
        assert(listen(sockfd, 128) == 0);
        host.hangRD();
        dbg("epoll_wait()");
        dbg(epoll_wait(epolltree.getfd(), epolltree.eventsList, MAX_CLIENTS, -1) );
    }

private:
    int sockfd;
    struct sockaddr_in servAddr, cliAddr;
    epoll epolltree;
};

#endif