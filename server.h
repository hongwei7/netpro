#ifndef __NETPRO__SERVER__H
#define __NETPRE__SERVER__H

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
#include "tcpconn.h"
#include "threadPool.h"
#include "epoll.h"
#include "dbg.h"

const int DEFAULT_POOL_MIN = 10;
const int DEFAULT_POOL_MAX = 1000;
const int DEFAULT_TASK_NUM = 50;

void defaultRead(char *buf, int size, tcpconn *wk)
{
    dbg(buf);
}
char *defaultWrite(int *size, tcpconn *wk)
{
    dbg("default write back function");
    return nullptr;
}
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
    server(int port, void (*raction)(char *, int, tcpconn *), char *(*waction)(int *, tcpconn *))
        : sockfd(socket(AF_INET, SOCK_STREAM, 0)), readAction(raction), writeAction(waction),
          pool(DEFAULT_POOL_MIN, DEFAULT_POOL_MAX, DEFAULT_TASK_NUM)
    {
        if (readAction == nullptr)
            readAction = defaultRead;
        if (writeAction == nullptr)
            writeAction = defaultWrite;
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
        tcpconn* sockTcp = new tcpconn (sockfd, nullptr, nullptr, &epolltree, &tcpconnsListMutex);
        auto ev = new event<tcpconn>(&epolltree, sockfd, sockTcp, false);
        // tcpconnsList.push_back(*ev->sharedPtr);
        tcpMap[sockfd] = *ev->sharedPtr;
    }
    void mainloop()
    {
        // std::vector<std::weak_ptr<event<tcpconn>>> sPtrVec;
        while (true)
        {
            // dbg("epoll wait");


            int ret = epoll_wait(epolltree.getepfd(), epolltree.eventsList, MAX_CLIENTS, -1);
            assert(ret > 0);
            dbg(ret);

            // for(int i = 0; i < ret; ++i){
            //     auto sptr = ((event<tcpconn>*)epolltree.eventsList[i].data.ptr)->sharedPtr;
            //     sPtrVec.push_back(*sptr);
            //     dbg(sPtrVec.size());
            // }

            // dbg("accept");
            for (int i = 0; i < ret; ++i)
            {
                dbg("deal with event");
                // dbg(sPtrVec[i]->getfd());
                event<tcpconn> *ptr = (event<tcpconn>*)epolltree.eventsList[i].data.ptr;
                
                dbg(ptr->getfd());
                if (ptr->getfd() == sockfd)
                    newClient();
                else
                {
                    if(tcpMap.find(ptr->getfd()) == tcpMap.end()){
                        dbg("-----");
                        dbg(ptr->getfd());
                        continue;
                    }
                    tcpMap.erase(ptr->getfd());
                    dbg("deal with event");
                    // dbg(tcpconnsList.size());
                    epolltree.eventsList[i];
                    dbg("BEFORE DEAL");
                    std::shared_ptr<event<tcpconn>> sharedPtr(*ptr->sharedPtr);
                    pool.threadPoolAdd(dealWithClient, (void*) &(epolltree.eventsList[i]));
                    // dealWithClient((void*)&epolltree.eventsList[i]);
                    // tcpconnsList.remove(sharedPtr);
                    dbg("MAIN LOOP END");
                }
            }

        }
    }
    void newClient()
    {
        dbg("NEW CLIENT");
        socklen_t socksize = sizeof(cliAddr);
        int clifd = accept(sockfd, (sockaddr *)&cliAddr, &socksize);
        assert(clifd > 0);
        setNonBlock(clifd);
        // dbg(clifd);
        tcpconn* newcli = new tcpconn(clifd, readAction, writeAction, &epolltree, &tcpconnsListMutex);
        // dbg(tcpconnsList.size());
        auto ev = new event<tcpconn>(&epolltree, clifd, newcli, true);
        // tcpconnsList.push_back(*ev->sharedPtr);
        tcpMap[clifd] = *ev->sharedPtr;
    }

    virtual ~server()
    {
        // tcpconnsList.clear();
    }

private:
    int sockfd;
    struct sockaddr_in servAddr, cliAddr;
    epoll epolltree;
    void (*readAction)(char *, int, tcpconn *);
    char *(*writeAction)(int *, tcpconn *);
    // std::list<std::shared_ptr<event<tcpconn>>> tcpconnsList;
    std::map<int, std::shared_ptr<event<tcpconn>>> tcpMap;
    mutex tcpconnsListMutex;
    threadPool pool;
};

#endif
