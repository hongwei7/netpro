#ifndef __NETPRO__SERVER__H
#define __NETPRE__SERVER__H

#include<sys/socket.h>
#include <list>
#include<assert.h>
#include<sys/types.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include <stdlib.h>
#include <unistd.h>
#include "tcpconn.h"
#include "threadPool.h"
#include "epoll.h"
#include "dbg.h"

const int DEFAULT_POOL_MIN = 3;
const int DEFAULT_POOL_MAX = 1000;
const int DEFAULT_TASK_NUM = 20;

void defaultRead(char* buf, int size, tcpconn* wk) {
    dbg(buf);
}
char* defaultWrite(int* size, tcpconn* wk) {
    dbg("default write back function");
    wk->ListenRead();
    return nullptr;
}
int setNonBlock(int sock) {
    int flags;
    flags = fcntl(sock, F_GETFL, 0);
    flags |= O_NONBLOCK;
    flags |= O_NDELAY;
    int ret = fcntl(sock, F_SETFL, flags);
    return ret;
}


class server {
public:
    server(int port, void(*raction)(char*, int, tcpconn*), char*(*waction)(int*, tcpconn*))
        : sockfd(socket(AF_INET, SOCK_STREAM, 0)), readAction(raction), writeAction(waction),
        pool(DEFAULT_POOL_MIN, DEFAULT_POOL_MAX, DEFAULT_TASK_NUM) {
        if (readAction == nullptr)readAction = defaultRead;
        if (writeAction ==nullptr) writeAction = defaultWrite;
        assert(sockfd != 0);
        memset(&servAddr, 0, sizeof(servAddr));
        memset(&cliAddr, 0, sizeof(cliAddr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(port);
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        int opt = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        int oldSocketFlag = fcntl(sockfd, F_GETFL, 0);
        int newSocketFlag = oldSocketFlag | O_NONBLOCK;
        if (fcntl(sockfd, F_SETFL, newSocketFlag) == -1) {
            close(sockfd);
            perror("fcntl");
            exit(1);
        }

        assert(bind(sockfd, (sockaddr*)&servAddr, sizeof(servAddr)) == 0);
        assert(listen(sockfd, 128) == 0);
        tcpconnsListMutex.lock();
        tcpconnsList.push_back(new tcpconn(sockfd, nullptr, nullptr, &epolltree, nullptr, nullptr));
        tcpconnsListMutex.unlock();
    }
    void mainloop() {
        while (true) {
            dbg("epoll wait");

            int ret = epoll_wait(epolltree.getepfd(), epolltree.eventsList, MAX_CLIENTS, -1);
            assert(ret > 0);
            for (int i = 0; i < ret; ++i) {
                // dbg("deal with event");
                tcpconn* ptr = (tcpconn*)epolltree.eventsList[i].data.ptr;
                if (ptr->getfd() == sockfd) newClient();
                else {
                    pool.threadPoolAdd(dealWithClient, (void*)ptr);
                    //dealWithClient((void*)(ptr));
                }
            }
        }
    }
    void newClient() {
        dbg("newClient");
        socklen_t socksize = sizeof(cliAddr);
        int clifd = accept(sockfd, (sockaddr*)&cliAddr, &socksize);
        assert(clifd > 0);
        setNonBlock(clifd);
        tcpconnsListMutex.lock();
        tcpconnsList.push_back(new tcpconn(clifd, readAction, writeAction, &epolltree, &tcpconnsList, &tcpconnsListMutex));
        tcpconnsListMutex.unlock();
    }



    virtual ~server() {
        tcpconnsListMutex.lock();
        for (auto& cli : tcpconnsList) {
            delete cli;
        }
        tcpconnsListMutex.unlock();
    }

private:
    int sockfd;
    struct sockaddr_in servAddr, cliAddr;
    epoll epolltree;
    void(*readAction)(char*, int, tcpconn*);
    char*(*writeAction)(int*, tcpconn*);
    std::list<tcpconn*> tcpconnsList;
    mutex tcpconnsListMutex;
    threadPool pool;
};

#endif
