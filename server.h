#ifndef __NETPRO__SERVER__H
#define __NETPRO__SERVER__H


#include <sys/socket.h>
#include <list>
#include <map>
#include <assert.h>
#include <sys/types.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "tcpconn.h"
#include "threadPool.h"
#include "epoll.h"
#include "dbg.h"

const int DEFAULT_POOL_MIN = 5;
const int DEFAULT_POOL_MAX = 1000;
const int DEFAULT_TASK_NUM = 100;


int setNonBlock(int sock)
{
    int flags;
    flags = fcntl(sock, F_GETFL);
    flags |= O_NONBLOCK;
    int ret = fcntl(sock, F_SETFL, flags);
    return ret;
}

class server
{
public:
    server(int port): sockfd(socket(AF_INET, SOCK_STREAM, 0)),
        pool(DEFAULT_POOL_MIN, DEFAULT_POOL_MAX, DEFAULT_TASK_NUM)
    {

        tcpconn::tcpMap = &tcpMap;
        tcpconn::listMutex = &tcpconnsListMutex;
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
        if (fcntl(sockfd, F_SETFL, newSocketFlag) == -1)
        {
            close(sockfd);
            perror("fcntl");
            exit(1);
        }

        assert(bind(sockfd, (sockaddr *)&servAddr, sizeof(servAddr)) == 0);
        assert(listen(sockfd, 128) == 0);
        tcpconn* sockTcp = new tcpconn (sockfd);
        auto ev = new event<tcpconn>(&epolltree, sockfd, sockTcp, true);
        tcpMap[sockfd] = *ev->sharedPtr;
    }
    void mainloop()
    {
        while (true)
        {
            // dbg("epoll wait");


            int ret = -1;
            while(ret == -1){
                ret = epoll_wait(epolltree.getepfd(), epolltree.eventsList, MAX_CLIENTS, -1);
                dbg(ret);
                if(ret == -1)perror("epoll wait");
            }
            assert(ret >= 0);


            for (int i = 0; i < ret; ++i)
            {
                dbg("deal with event");
                int clifd = epolltree.eventsList[i].data.fd;
                if (clifd == sockfd){
                    dbg(clifd);
                    newClient();
                }
                else
                {
                    tcpconnsListMutex.lock();
                    auto iter = tcpMap.find(clifd);
                    if(iter == tcpMap.end()){
                        dbg("-----");
                        tcpconnsListMutex.unlock();
                        continue;
                    }

                    epolltree.eventsList[i];
                    std::shared_ptr<event<tcpconn>> sharedPtr(iter->second);
                    tcpconnsListMutex.unlock();
                    if(epolltree.eventsList[i].events & EPOLLIN){
                        pool.threadPoolAdd(dealWithClientRead, (void*) &clifd);
                        tcpconn::forwardRead.wait();
                    }
                    if(epolltree.eventsList[i].events & EPOLLOUT){
                        pool.threadPoolAdd(dealWithClientWrite, (void*) &clifd);
                        tcpconn::forwardWrite.wait();
                    }
                    // dealWithClient((void*)&epolltree.eventsList[i]);
                    dbg(sharedPtr.use_count());
                    dbg("MAIN LOOP END");
                }
            }

        }
    }
    void newClient()
    {
        dbg("------NEW CLIENT------");
        socklen_t socksize = sizeof(cliAddr);
        int clifd = accept(sockfd, (sockaddr *)&cliAddr, &socksize);
        assert(clifd > 0);
        setNonBlock(clifd);
        // dbg(clifd);
        tcpconn* newcli = new tcpconn(clifd);
        auto ev = new event<tcpconn>(&epolltree, clifd, newcli, false);
        tcpMap[clifd] = *ev->sharedPtr;
        delete ev->sharedPtr;
    }

    ~server(){
        tcpMap.clear();
    }


private:
    int sockfd;
    struct sockaddr_in servAddr, cliAddr;
    epoll epolltree;
    std::map<int, std::shared_ptr<event<tcpconn>>> tcpMap;
    mutex tcpconnsListMutex;
    threadPool pool;
};

#endif
