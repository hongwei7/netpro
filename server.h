#ifndef __NETPRO__SERVER__H
#define __NETPRO__SERVER__H


#include <sys/socket.h>
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

template<typename ConnectType = tcpconn>
class server
{
public:
    server(int port): sockfd(socket(AF_INET, SOCK_STREAM, 0)),
        pool(DEFAULT_POOL_MIN, DEFAULT_POOL_MAX, DEFAULT_TASK_NUM)
    {
        //屏蔽sigpipe错误
        sigset_t signal_mask;
        sigemptyset (&signal_mask);  
        sigaddset (&signal_mask, SIGPIPE);
        int rc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
        if (rc != 0) printf("block sigpipe error\n");

        ConnectType::connectMap = &connectMap;
        ConnectType::listMutex = &ConnectTypesListMutex;
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
        ConnectType* sockTcp = new ConnectType (sockfd);
        auto ev = new event<ConnectType>(&epolltree, sockfd, sockTcp, true);
        connectMap[sockfd] = std::shared_ptr<event<ConnectType>>(ev);
    }
    void mainloop()
    {
        while (true)
        {
            int ret = -1;
            while(ret == -1){
                ret = epoll_wait(epolltree.getepfd(), epolltree.eventsList, MAX_CLIENTS, -1);
                dbg(ret);
                if(ret == -1)perror("epoll wait");
            }
            assert(ret >= 0);


            for (int i = 0; i < ret; ++i)
            {
                dbg("DEAL WITH EVENT");
                int clifd = epolltree.eventsList[i].data.fd;
                if (clifd == sockfd){
                    dbg(clifd);
                    newClient();
                }
                else
                {
                    ConnectTypesListMutex.lock();
                    auto iter = connectMap.find(clifd);
                    if(iter == connectMap.end()){
                        dbg("-----");
                        ConnectTypesListMutex.unlock();
                        continue;
                    }

                    epolltree.eventsList[i];
                    std::shared_ptr<event<ConnectType>> sharedPtr(iter->second);
                    ConnectTypesListMutex.unlock();
                    if(epolltree.eventsList[i].events & EPOLLIN){
                        pool.threadPoolAdd(dealWithClientRead, (void*) &clifd);
                        ConnectType::forwardRead.wait();
                    }
                    if(epolltree.eventsList[i].events & EPOLLOUT){
                        pool.threadPoolAdd(dealWithClientWrite, (void*) &clifd);
                        ConnectType::forwardWrite.wait();
                    }
                    // dealWithClient((void*)&epolltree.eventsList[i]);
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
        ConnectType* newcli = new ConnectType(clifd);
        auto ev = new event<ConnectType>(&epolltree, clifd, newcli, false);
        connectMap[clifd] = std::shared_ptr<event<ConnectType>>(ev);
    }

    void destroy(){
        connectMap.clear();
    }

    ~server(){
        destroy();
    }


private:
    int sockfd;
    struct sockaddr_in servAddr, cliAddr;
    epoll epolltree;
    std::map<int, std::shared_ptr<event<ConnectType>>> connectMap;
    mutex ConnectTypesListMutex;
    threadPool pool;
};

#endif
